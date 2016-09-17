// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "site.h"
#include <QEventLoop>

#include "CommonUtils.h"
#include "errhlpr.h"


Site::Site(const QString &type, const QString &strDestDir, IMainLog *log,
           IFileSavedCallback *fileSavedClbk)
    : m_strDestDir(strDestDir), m_plugin(type, log), m_log(log),
      m_httpDwnld(NULL), m_serPicDwnld(NULL, log, fileSavedClbk)
{
    // bad_function_call exception in case of problem -
    m_siteInfo = m_plugin.GetSiteInfo();
    m_urlBuilder = m_plugin.GetUrlBuilder();
    m_fileNameBuilder = m_plugin.GetFileNameBuilder();

    m_serPicDwnld.SetSite(this);
    m_serPicDwnld.SetDestinationFolder(strDestDir);

    m_db = ISqLiteManagerCtr(m_strDestDir + QsFrWs(m_siteInfo->GetDBFileName()),
                             QsFrWs(m_siteInfo->GetDBTableName()), m_log);
}

std::shared_ptr<IHtmlPageElm> Site::HtmlPageElmCtr(
        const QString &strContent) const
{
    return m_plugin.GetHtmlPageElm(strContent);
}

ISqLiteManager *Site::DB()
{
    return m_db.data();
}

ISerialPicsDownloader *Site::SerialPicsDwnld()
{
    return &m_serPicDwnld;
}

bool Site::DownloadPicLoopWithWait(
        const QMap<QString, QString> &mapPicPageUrlFileName)
{
    const int iConcurrentDownload = 5;
    QMap<QString, QString> subMap;
    for (auto i = mapPicPageUrlFileName.constBegin();
         i != mapPicPageUrlFileName.constEnd();) {
        subMap[i.key()] = i.value();
        ++i;
        if (subMap.size() >= iConcurrentDownload ||
                i == mapPicPageUrlFileName.constEnd()) {
            m_serPicDwnld.AddPicPageUrlLstToQueue(subMap);
            QEventLoop waitLoop;
            QObject::connect(&m_serPicDwnld, SIGNAL(QueueEmpty()),
                    &waitLoop, SLOT(quit()));
            waitLoop.exec();
            subMap.clear();
        }
    }
    auto errPicLst = m_serPicDwnld.GetAndClearErrPicsList();
    return errPicLst.empty();
}

bool Site::ProcessUser(const QString &strUserId, const QString &strActivTime)
{
    m_log->LogOut("Process " + strUserId + "... ", true);

    auto strMainPageUrl = QsFrWs(UrlBldr()->GetMainUserPageUrlById(
                                     strUserId.toStdWString()));
    auto byteRep = m_httpDwnld.DownloadSync(QUrl(strMainPageUrl));
    QString strRep = CommonUtils::Win1251ToQstring(byteRep);
    if (strRep.isEmpty()) {
        if (CErrHlpr::IgnMsgBox("Could not download user main page",
                                "URL: " + strMainPageUrl)) {
            return true;
        }
        return false;
    }

    auto htmlElmt(HtmlPageElmCtr(strRep));
    Q_ASSERT(!htmlElmt->IsSelfNamePresent());
    QString strUserName( m_siteInfo->UsersIdAndNameSame() ?
                             "" : QsFrWs(htmlElmt->GetUserName()));

    QString strLastActiveTime;
    QMap<QString, QString> mapPicPageUrlFileName;
    try {
        strLastActiveTime = QsFrWs(htmlElmt->GetLastActivityTime());
        if(strLastActiveTime == strActivTime) {
            m_log->LogOut(QString("%1 last active time match - switch to next")
                          .arg(strUserName));
            return true;
        }

        m_log->LogOut(QString("%1 last active time does not match(\"%2\" vs"
                              " \"%3\") - parse common album")
                      .arg(strUserName)
                      .arg(strLastActiveTime).arg(strActivTime));

        auto albManager(IAlbmMngrCtr(this, m_log, m_strDestDir,
                                     strUserId, htmlElmt, &m_httpDwnld));
        mapPicPageUrlFileName = albManager->GetMissingPicPageUrlLst();
    } catch (const parse_ex &ex) {
        if (CErrHlpr::IgnMsgBox("Parse error: " + QsFrWs(ex.strWhat()),
                                QsFrWs(ex.strDetails()))) {
            return true;
        }
        return false;
    }

    if (!mapPicPageUrlFileName.isEmpty()) {
        if (!DownloadPicLoopWithWait(mapPicPageUrlFileName)) {
            if (CErrHlpr::IgnMsgBox(
                        "Could not download pics",
                        "User: " + QsFrWs(htmlElmt->GetUserName()))) {
                return true;
            }
            return false;
        }
    }
    m_log->LogOut("Update last active time ("
                  + QsFrWs(htmlElmt->GetUserName()) + ")");
    DB()->UpdateLastActivityTime(strUserId, strLastActiveTime);
    return true;
}
