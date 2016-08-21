// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "albummanager.h"
#include <QFile>

#include "errhlpr.h"
#include "CommonUtils.h"

QSharedPointer<IAlbmMngr> IAlbmMngrCtr(
        ISite *pSite, IMainLog *pLog, const QString &strFolder,
        const QString &strUserId,
        const QSharedPointer<IHtmlPageElm> &htmlElmUserMain,
        HttpDownloader &pHttpDown)
{
    QSharedPointer<IAlbmMngr> retVal(new AlbumManager(pSite, pLog, strFolder,
                                                      strUserId,
                                                      htmlElmUserMain,
                                                      pHttpDown));
    return retVal;
}

AlbumManager::AlbumManager(ISite *pSite, IMainLog *pLog,
                           const QString &strFolder, const QString &strUserId,
                           const QSharedPointer<IHtmlPageElm> &htmlElmUserMain,
                           HttpDownloader &pHttpDown)
    : m_pSite(pSite), m_pLog(pLog), m_strFolder(strFolder),
      m_strUserId(strUserId), m_htmlElmUsrMain(htmlElmUserMain),
      m_pHttpDown(pHttpDown)
{
    if (m_strFolder.lastIndexOf('/') != m_strFolder.size() - 1) {
        m_strFolder += '/';
    }
}

qListPairOf2Str AlbumManager::GetMissingPicPageUrlLst()
{
    qListPairOf2Str lstUrlFileName;

    LogOut(QString("Start to process album of user %1(%2) with total %3, hide"
                   " %4, limited %5 pics")
           .arg(m_strUserId).arg(m_htmlElmUsrMain->GetUserName())
           .arg(m_htmlElmUsrMain->GetTotalPhotoCount_Method2())
           .arg(m_htmlElmUsrMain->GetHidePhotoCount())
           .arg(m_htmlElmUsrMain->GetViewLimitedPhotoCount()));

    QString strAlbumLink = m_htmlElmUsrMain->GetFirstCommonAlbumUrl();
    if (strAlbumLink.isEmpty()) {
        strAlbumLink = m_pSite->UrlBldr()->GetCommonAlbumUrlById(m_strUserId);

        LogOut(QString("WARNING - empty common album for user %1(%2). "
                       "Try direct common album link %3")
               .arg(m_strUserId).arg(m_htmlElmUsrMain->GetUserName())
               .arg(strAlbumLink));
    }
    for (;
        strAlbumLink != "" ;
        strAlbumLink = m_htmlElmUsrMain->GetNextCommonAlbumUrl(strAlbumLink))
    {
        QByteArray byteRep = m_pHttpDown.DownloadSync(QUrl(strAlbumLink));
        QString strRep = CommonUtils::Win1251ToQstring(byteRep);

        if (strRep.isEmpty()) {
            if (CErrHlpr::IgnMsgBox("Could not download album page for ID "
                                    + m_strUserId, "URL: " + strAlbumLink)) {
                continue;
            }
            return qListPairOf2Str();
        }

        auto htmlAlbumPage(m_pSite->HtmlPageElmCtr(strRep));
        auto lstUserAlbumPic(htmlAlbumPage->GetPicPageUrlsListByImageIdOnly());
        if (lstUserAlbumPic.isEmpty()) {
            break;
        }

        int iSzBeforeForeach(lstUrlFileName.size());

        QString strPicUrl;
        foreach(strPicUrl, lstUserAlbumPic) {
            auto strPicFileName = m_pSite->FileNameBldr()->GetPicFileName(
                        m_strUserId,
                        m_pSite->UrlBldr()->GetPicIdFromUrl(strPicUrl));

            QFile file(m_strFolder + strPicFileName);
            if (!file.exists()) {
                Q_ASSERT(!strPicUrl.isEmpty() && !strPicFileName.isEmpty());
                lstUrlFileName.append(qMakePair(strPicUrl, strPicFileName));
            } else {
                LogOut("The " + file.fileName()
                       + " exist, break parsing album pages");
                break;
            }
        }
        if (lstUserAlbumPic.size() !=
                lstUrlFileName.size() - iSzBeforeForeach) {
            break;
        } else {
            LogOut("User id " + m_strUserId +
                   " switch to next album page(or end)");
        }
    }

    LogOut(QString("Finished(user id %1). %2 from %3 picture is missing local")
           .arg(m_strUserId).arg(lstUrlFileName.size())
           .arg(m_htmlElmUsrMain->GetTotalPhotoCount_Method2()));

    return lstUrlFileName;
}

void AlbumManager::LogOut(const QString &strMessage)
{
    if (m_pLog) {
        m_pLog->LogOut("---[AlbumManager] " + strMessage);
    }
}
