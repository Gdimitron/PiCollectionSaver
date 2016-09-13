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

    m_db = ISqLiteManagerCtr(m_strDestDir + "/" +
                             QsFrWs(m_siteInfo->GetDBFileName()),
                             QsFrWs(m_siteInfo->GetDBTableName()), m_log);
}

std::shared_ptr<IHtmlPageElm> Site::HtmlPageElmCtr(const QString &strContent)
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

bool Site::DownloadPicLoopWithWait(const qListPairOf2Str &picPageLinkFileName)
{
    const int iConcurrentDownload = 10;
    for (int i = 0; i < picPageLinkFileName.size(); i += iConcurrentDownload) {
        auto aListPart = picPageLinkFileName.mid(i, iConcurrentDownload);
        m_serPicDwnld.AddPicPageUrlLstToQueue(aListPart);

        QEventLoop waitLoop;
        QObject::connect(&m_serPicDwnld, SIGNAL(QueueEmpty()),
                &waitLoop, SLOT(quit()));
        waitLoop.exec();
    }
    auto errPicLst = m_serPicDwnld.GetAndClearErrPicsList();
    return errPicLst.empty();
}

bool Site::ProcessUser(QPair<QString, QString> prUsrsActvTime)
{
    m_log->LogOut("Process " + prUsrsActvTime.first);

    auto strMainPageUrl = QsFrWs(UrlBldr()->GetMainUserPageUrlById(
                prUsrsActvTime.first.toStdWString()));
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

    QString strLastActiveTime;
    qListPairOf2Str qlstPrPicPageLinkFileName;
    try {
        strLastActiveTime = QsFrWs(htmlElmt->GetLastActivityTime());
        if(strLastActiveTime == prUsrsActvTime.second) {
            m_log->LogOut("Last active time match(" +
                          QsFrWs(htmlElmt->GetUserName()) + ") switch to next");
            return true;
        }

        m_log->LogOut(QString("Last active time does not match(%1 \"%2\" vs"
                              " \"%3\") - parse common album")
                      .arg(QsFrWs(htmlElmt->GetUserName()))
                      .arg(strLastActiveTime).arg(prUsrsActvTime.second));

        auto albManager(IAlbmMngrCtr(this, m_log, m_strDestDir,
                                     prUsrsActvTime.first, htmlElmt,
                                     m_httpDwnld));
        qlstPrPicPageLinkFileName = albManager->GetMissingPicPageUrlLst();
    } catch (const parse_ex &ex) {
        if (CErrHlpr::IgnMsgBox("Parse error: " + QsFrWs(ex.strWhat()),
                                QsFrWs(ex.strDetails()))) {
            return true;
        }
        return false;
    }

    if (!qlstPrPicPageLinkFileName.isEmpty()) {
        if (!DownloadPicLoopWithWait(qlstPrPicPageLinkFileName)) {
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
    prUsrsActvTime.second = strLastActiveTime;
    DB()->UpdateLastActivityTime(prUsrsActvTime);

    return true;
}
