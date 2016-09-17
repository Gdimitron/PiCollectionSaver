// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "albummanager.h"
#include <QFile>

#include "errhlpr.h"
#include "CommonUtils.h"
#include "CommonConstants.h"
#include "HttpDownloader.h"

QSharedPointer<IAlbmMngr> IAlbmMngrCtr(
        ISite *pSite, IMainLog *pLog, const QString &strFolder,
        const QString &strUserId,
        const std::shared_ptr<IHtmlPageElm> &htmlElmUserMain,
        HttpDownloader *pHttpDown)
{
    QSharedPointer<IAlbmMngr> retVal(new AlbumManager(pSite, pLog, strFolder,
                                                      strUserId,
                                                      htmlElmUserMain,
                                                      pHttpDown));
    return retVal;
}

AlbumManager::AlbumManager(ISite *pSite, IMainLog *pLog,
                           const QString &strFolder, const QString &strUserId,
                           const std::shared_ptr<IHtmlPageElm> &htmlElmUserMain,
                           HttpDownloader *pHttpDown)
    : m_pSite(pSite), m_pLog(pLog), m_strFolder(strFolder),
      m_strUserId(strUserId), m_htmlElmUsrMain(htmlElmUserMain),
      m_pHttpDown(pHttpDown)
{
}

QMap<QString, QString> AlbumManager::GetMissingPicPageUrlLst()
{
    QMap<QString, QString> res;

    LogOut(QString("Start to process album of user %1(%2) with total %3, hide"
                   " %4, limited %5 pics")
           .arg(m_strUserId).arg(QsFrWs(m_htmlElmUsrMain->GetUserName()))
           .arg(m_htmlElmUsrMain->GetTotalPhotoCount())
           .arg(m_htmlElmUsrMain->GetHidePhotoCount())
           .arg(m_htmlElmUsrMain->GetViewLimitedPhotoCount()));

    QString strAlbumLink = QsFrWs(m_htmlElmUsrMain->GetFirstCommonAlbumUrl());
    if (strAlbumLink.isEmpty()) {
        strAlbumLink = QsFrWs(m_pSite->UrlBldr()->GetCommonAlbumUrlById(
                                  m_strUserId.toStdWString()));

        LogOut(QString("WARNING - empty common album for user %1(%2). "
                       "Try direct common album link %3")
               .arg(m_strUserId).arg(QsFrWs(m_htmlElmUsrMain->GetUserName()))
               .arg(strAlbumLink));
    }
    for (int iPicOnPageCnt = 0;
         strAlbumLink != "" ;
         strAlbumLink = QsFrWs(m_htmlElmUsrMain->GetNextCommonAlbumUrl(
                                  strAlbumLink.toStdWString(), iPicOnPageCnt)))
    {
        QByteArray byteRep = m_pHttpDown->DownloadSync(QUrl(strAlbumLink));
        QString strRep = CommonUtils::Win1251ToQstring(byteRep);

        if (strRep.isEmpty()) {
            if (CErrHlpr::IgnMsgBox("Could not download album page for ID "
                                    + m_strUserId, "URL: " + strAlbumLink)) {
                continue;
            }
            return QMap<QString, QString>();
        }

        auto htmlAlbumPage(m_pSite->HtmlPageElmCtr(strRep));
        auto lstUserAlbumPic(htmlAlbumPage->GetPicPageUrlsList());
        if (lstUserAlbumPic.empty()) {
            break;
        }

        int iSzBeforeForeach(res.size());

        for(const std::wstring &wStrPicUrl: lstUserAlbumPic) {
            auto strUrl = QsFrWs(wStrPicUrl);
            auto strFile = QsFrWs(m_pSite->FileNameBldr()->GetPicFileNameWoExt(
                                      m_strUserId.toStdWString(),
                                      m_pSite->UrlBldr()->
                                      GetPicIdFromUrl(strUrl.toStdWString())));

            QString fileExist;
            for (auto ext: g_imgExtensions) {
                auto name(strFile + QsFrWs(ext));
                if (QFile::exists(m_strFolder + name)) {
                    fileExist = name;
                    break;
                }
            }
            if (fileExist.isEmpty()) {
                Q_ASSERT(!strUrl.isEmpty() && !strFile.isEmpty());
                res[strUrl] = strFile;
            } else {
                LogOut("The " + fileExist + " exist, break parsing album page");
                break;
            }
        }
        iPicOnPageCnt = static_cast<int>(lstUserAlbumPic.size());
        if (iPicOnPageCnt != res.size() - iSzBeforeForeach) {
            break;
        } else {
            LogOut(QString("From current page added %1 pic, switch to next "
                           "album page(or end)").arg(iPicOnPageCnt));
        }
    }

    LogOut(QString("Finished(ID %1). %2 from %3 pictures are missing local")
           .arg(m_strUserId).arg(res.size())
           .arg(m_htmlElmUsrMain->GetTotalPhotoCount()));

    return res;
}

void AlbumManager::LogOut(const QString &strMessage)
{
    if (m_pLog) {
        m_pLog->LogOut("---[AlbumManager] " + strMessage);
    }
}
