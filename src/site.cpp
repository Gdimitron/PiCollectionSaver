// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "site.h"
#include <QEventLoop>

#include "CommonConstants.h"
#include "CommonUtils.h"
#include "errhlpr.h"


Site::Site(const QString &type, const QString &strDestDir, IMainLog *log,
           IFileSavedCallback *fileSavedClbk)
    : m_strDestDir(strDestDir), m_log(log), m_DB(NULL), m_httpDwnld(NULL),
      m_serPicDwnld(NULL, log, fileSavedClbk)
{
    m_PlugLib.setFileName(QString("../libPiCollection%1.so").arg(type));
    if (!m_PlugLib.load()) {
        m_log->LogOut("Failed to load lib " + m_PlugLib.fileName() +  ". Error:"
                      + m_PlugLib.errorString());
        throw std::bad_function_call();
    }
    f_ISiteInfoCtr fSiteInfo((ISiteInfoCtr_t*)m_PlugLib
                             .resolve("ISiteInfoCtr"));
    f_IFileSysBldrCtr fBldCtr((IFileSysBldrCtr_t*)m_PlugLib
                              .resolve("IFileSysBldrCtr"));
    f_IUrlBuilderCtr fUrlBldr((IUrlBuilderCtr_t*)m_PlugLib
                              .resolve("IUrlBuilderCtr"));
    m_fHtmlPageElm = ((IHtmlPageElmCtr_t*)m_PlugLib.resolve("IHtmlPageElmCtr"));
    if (!m_fHtmlPageElm) {
        throw std::bad_function_call();
    }

    // bad_function_call exception in case of problem -
    m_siteInfo = fSiteInfo();
    m_fileNameBuilder = fBldCtr();
    m_urlBuilder = fUrlBldr();

    m_serPicDwnld.SetSite(this);
    m_serPicDwnld.SetDestinationFolder(strDestDir);
}

QSharedPointer<ISiteInfo> Site::SiteInfo()
{
    return m_siteInfo;
}


QSharedPointer<IUrlBuilder> Site::UrlBldr()
{
    return m_urlBuilder;
}

QSharedPointer<IFileSysBldr> Site::FileNameBldr()
{
    return m_fileNameBuilder;
}

QSharedPointer<IHtmlPageElm> Site::HtmlPageElmCtr(const QString &strContent)
{
    return m_fHtmlPageElm(strContent);
}

QSharedPointer<ISqLiteManager> Site::DB()
{
    if (m_DB.isNull()) {
        m_DB = ISqLiteManagerCtr(m_strDestDir +"/"+ m_siteInfo->GetDBFileName(),
                                 m_siteInfo->GetDBTableName());
    }
    return m_DB;
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

    QString strMainPageUrl = UrlBldr()->GetMainUserPageUrlById(
                prUsrsActvTime.first);
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
        strLastActiveTime = htmlElmt->GetLastActivityTime();
        if(strLastActiveTime == prUsrsActvTime.second) {
            m_log->LogOut("Last active time match(" + htmlElmt->GetUserName()
                   + ") - switch to next");
            return true;
        }

        m_log->LogOut(QString("Last active time does not match(%1 \"%2\" vs \"%3\")"
                       " - parse common album").arg(htmlElmt->GetUserName())
               .arg(strLastActiveTime).arg(prUsrsActvTime.second));

        auto albManager(IAlbmMngrCtr(this, m_log, m_strDestDir,
                                     prUsrsActvTime.first, htmlElmt, m_httpDwnld));
        qlstPrPicPageLinkFileName = albManager->GetMissingPicPageUrlLst();
    } catch (const parse_ex &ex) {
        if (CErrHlpr::IgnMsgBox("Parse error: " + *ex.strWhat(),
                                *ex.strDetails())) {
            return true;
        }
        return false;
    }

    if (!qlstPrPicPageLinkFileName.isEmpty()) {
        if (!DownloadPicLoopWithWait(qlstPrPicPageLinkFileName)) {
            if (CErrHlpr::IgnMsgBox("Could not download pics",
                                    "User: " + htmlElmt->GetUserName())) {
                return true;
            }
            return false;
        }
    }
    m_log->LogOut("Update last active time (" + htmlElmt->GetUserName() + ")");
    prUsrsActvTime.second = strLastActiveTime;
    DB()->UpdateLastActivityTime(prUsrsActvTime);

    return true;
}

