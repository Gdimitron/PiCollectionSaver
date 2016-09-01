// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "SerialPicsDownloader.h"
#include <QFile>

#include "CommonConstants.h"
#include "CommonUtils.h"


SerialPicsDownloader::SerialPicsDownloader(QObject *parent, IMainLog *pLog,
                                           IFileSavedCallback *pFileSaved)
    : QObject(parent), m_pHttp(new HttpDownloader(this)), m_bOverwrite(false),
      m_pSite(NULL), m_pLog(pLog), m_pFileSavedClbk(pFileSaved),
      m_iCurrentMaxOrderNumber(0)
{
    connect(m_pHttp, SIGNAL(done(int, const QUrl&, const QByteArray&)),
            this, SLOT(httpSlotDone(int, const QUrl&, const QByteArray&)));

    connect(m_pHttp, SIGNAL(error(const QUrl&, const QString&)),
            this, SLOT(httpSlotError(const QUrl&, const QString&)));
}

SerialPicsDownloader::~SerialPicsDownloader(void)
{
}

void SerialPicsDownloader::SetSite(ISite *pSite)
{
    m_pSite = pSite;
    Q_ASSERT(m_pHttp);
    m_pHttp->ObtainAuthCookie(
                QsFrWs(m_pSite->SiteInfo()->GetProtocolHostName()),
                QsFrWs(m_pSite->SiteInfo()->GetAuthInfo()));
}

void SerialPicsDownloader::SetOverwriteMode(bool bOverwrite)
{
    m_bOverwrite = bOverwrite;
}

void SerialPicsDownloader::SetDestinationFolder(const QString& strDstFolderPath)
{
    m_strDestinationFolderPath = strDstFolderPath;
    if (strDstFolderPath.lastIndexOf('/') != strDstFolderPath.size() - 1) {
        m_strDestinationFolderPath += '/';
    }
}

void SerialPicsDownloader::AddPicPageUrlLstToQueue(
        qListPairOf2Str &lstPicPageUrlFileName)
{
    foreach(auto pairElement, lstPicPageUrlFileName) {
        AddElementToList(pairElement.first, pairElement.second);
    }
    ListDownloadProcessingImpl();
}

qListPairOf2Str SerialPicsDownloader::GetAndClearErrPicsList()
{
    qListPairOf2Str ret;
    foreach(auto it, m_lstMainPicsErrList) {
        ret.append(QPair<QString, QString>(it.m_strPicPageUrl, it.m_strError));
    }
    m_lstMainPicsErrList.clear();
    return ret;
}

void SerialPicsDownloader::LogOut(const QString &strMessage)
{
    if (m_pLog) {
        m_pLog->LogOut("------[SerialPicsDownloader] " + strMessage);
    }
}

void SerialPicsDownloader::AddElementToList(const QString &strPicPageUrl,
                                            const QString &strFileNameToSave)
{
    QMutexLocker locker(&m_mutexForList);

    /*QDebug(QtDebugMsg) << "AddElementToList; strDirectPicUrl = "
                       << strPicPageUrl
                       << " strFileNameToSave = " << strFileNameToSave;*/

    // paranoid check - do not to download same
    for (auto it = m_lstMainPicsList.cbegin();
         it != m_lstMainPicsList.cend();
         ++it) {
        if (it->m_strPicPageUrl == strPicPageUrl ||
                it->m_strFileNameToSave == strFileNameToSave) {
            Q_ASSERT(false);
            return;
        }
    }

    stPicElement picElement(m_iCurrentMaxOrderNumber++);
    picElement.m_strPicPageUrl = strPicPageUrl;
    picElement.m_strFileNameToSave = strFileNameToSave;

    m_lstMainPicsList.push_back(picElement);
}

void SerialPicsDownloader::ListDownloadProcessingImpl(void)
{
    QMutexLocker locker(&m_mutexForList);

    for (auto it = m_lstMainPicsList.begin();
         it != m_lstMainPicsList.end();
         ++it) {
        if (it->m_bDownloading == false) {
            m_pHttp->DownloadAsync(it->m_strPicPageUrl);
            it->m_bDownloading = true;
        }
    }
}

void SerialPicsDownloader::httpSlotError(const QUrl& url, const QString& strErr)
{
    QDebug(QtDebugMsg) << "httpSlotError; ThredId = "
                       << QThread::currentThreadId()
                       << " Error = " << strErr;

    ListDownloadErrorProcess(url.toString(), strErr);
}

void SerialPicsDownloader::httpSlotDone(int iRplStatusCode, const QUrl& url,
                                        const QByteArray& byteArr)
{
    /*QDebug(QtDebugMsg) << "httpSlotDone; thred = "
                       << QThread::currentThreadId()
                       << " Status code = " << iRplStatusCode
                       << " Qurl = " << url;*/

    if (m_strDestinationFolderPath.isEmpty()) {
        ListDownloadErrorProcess(url.toString(), "Dest folder is empty.");
        return;
    }

    if (iRplStatusCode == 200) {
        if (url.toString().indexOf(
                    QsFrWs(m_pSite->SiteInfo()->PagePicUrlSign())) != -1) {
            PicPageDownloadDoneProcess(url.toString(), byteArr,
                                       iRplStatusCode);
            ProcessDoneItems();
        } else if (url.toString().indexOf(
                       QsFrWs(m_pSite->SiteInfo()->DirectPicUrlSign())) != -1) {
            DirectPicDownloadDoneProcess(url.toString(), byteArr,
                                         iRplStatusCode);
            ProcessDoneItems();
        } else {
            Q_ASSERT(false);
        }
    } else {
        QString strError("Error, not success HTTP reply code("
                         + QString::number(iRplStatusCode) + ")");
        ListDownloadErrorProcess(url.toString(), strError);
    }
}

void SerialPicsDownloader::PicPageDownloadDoneProcess(
        const QString &strPicPageUrl, const QByteArray& arrPicPage,
        int iHttpReplyCode)
{
    QMutexLocker locker(&m_mutexForList);

    for (auto it = m_lstMainPicsList.begin();
         it != m_lstMainPicsList.end();
         ++it) {
        if (it->m_strPicPageUrl == strPicPageUrl) { // find which is done
            it->m_ptrHtmlElmtPicPage = m_pSite->HtmlPageElmCtr(
                        CommonUtils::Win1251ToQstring(arrPicPage));
            it->m_iHttpReplyStatusCode = iHttpReplyCode;
            try { it->m_strDirectPicUrl = QsFrWs(it->m_ptrHtmlElmtPicPage->
                                                 GetBestPossibleDirectPicUrl());
            } catch (const parse_ex &ex) {
                QDebug(QtDebugMsg)
                        << "PicPageDownloadDoneProcess; ThredId = "
                        << QThread::currentThreadId()
                        << " Could not parse picture page for strPicPageUrl = "
                        << strPicPageUrl;

                it->m_bDonwloadError = true;
                it->m_strError = "Couldn't parse picture url: " +
                        QsFrWs(ex.strWhat());
                return;
            }
            Q_ASSERT(!it->m_strDirectPicUrl.isEmpty());
            QDebug(QtDebugMsg) << "PicPageDownloadDoneProcess; ThredId = "
                               << QThread::currentThreadId()
                               << " strUrl = " << it->m_strDirectPicUrl;
            m_pHttp->DownloadAsync(it->m_strDirectPicUrl);
            return;
        }
    }
    Q_ASSERT(false);
}

void SerialPicsDownloader::DirectPicDownloadDoneProcess(
        const QString &strDirectPicUrl, const QByteArray& arrPic,
        int iHttpReplyCode)
{
    QMutexLocker locker(&m_mutexForList);

    for (auto it = m_lstMainPicsList.begin();
         it != m_lstMainPicsList.end();
         ++it) {
        if (it->m_strDirectPicUrl == strDirectPicUrl) {	// find which is done
            if (arrPic.size() < 12 * 1024) {
                if (!it->m_bPicForBrowser) {
                    LogOut("WARNING: Image size for Url \""
                           + it->m_strPicPageUrl
                           + "\" is less than 12KB. "
                             "Try to obtain pic for browser");

                    it->m_bPicForBrowser = true;

                    it->m_strDirectPicUrl = QsFrWs(
                                it->m_ptrHtmlElmtPicPage->
                                GetShownInBrowserDirectPicUrl());
                    Q_ASSERT(!it->m_strDirectPicUrl.isEmpty());

                    m_pHttp->DownloadAsync(it->m_strDirectPicUrl);
                    return;
                }
                LogOut("WARNING: Two methods have been tried, but size"
                       "for Url \"" + it->m_strPicPageUrl
                       + "\" is less than 12KB. Saved as is.");
            }
            it->m_arrPicContent = arrPic;
            it->m_iHttpReplyStatusCode = iHttpReplyCode;
            it->m_bDownloadDone = true;
            break;
        }
    }
}

void SerialPicsDownloader::ListDownloadErrorProcess(const QString &strUrl,
        const QString &strError)
{
    QMutexLocker locker(&m_mutexForList);

    LogOut("Download error. Url \"" + strUrl
           + "\". Error message \"" + strError + "\".");

    for (auto it = m_lstMainPicsList.begin();
         it != m_lstMainPicsList.end();
         ++it) {
        if (it->m_strDirectPicUrl == strUrl
                || it->m_strPicPageUrl == strUrl) {
            it->m_bDonwloadError = true;
            it->m_strError = strError;
            break;
        }
    }
}

void SerialPicsDownloader::ProcessDoneItems()
{
    QMutexLocker locker(&m_mutexForList);

    for (auto it = m_lstMainPicsList.begin();
         it != m_lstMainPicsList.end();) {
        if (it->m_bDownloadDone == true) {
            // download done - save and erase from list
            SaveByteArrayToDisk(it->m_arrPicContent, m_strDestinationFolderPath
                                + it->m_strFileNameToSave);

            /*QDebug(QtDebugMsg) << "ListSerialDiskSaveImpl; ERASE ThredId = "
                               << QThread::currentThreadId()
                               << " PicPageUrl = " << it->m_strPicPageUrl
                               << " DirectPicUrl = " << it->m_strDirectPicUrl;*/

            it = m_lstMainPicsList.erase(it);
            continue;
        } else if (it->m_bDonwloadError == true) {
            LogOut("Download error. Url \"" + it->m_strPicPageUrl
                   + "\". Error message \"" + it->m_strError);
            m_lstMainPicsErrList.append(*it);
            it = m_lstMainPicsList.erase(it);
            continue;
        } else {
            // if next is not done yet - break(we need to save element in same
            // order as in list, (file time correspond position);
            break;
        }
    }
    if(m_lstMainPicsList.isEmpty()) {
        emit QueueEmpty();
    }
}

void SerialPicsDownloader::SaveByteArrayToDisk(const QByteArray& arrPic,
                                               const QString& strFilePath)
{
    QFile file(strFilePath);
    if (file.exists() && m_bOverwrite) {
        QFile::rename(strFilePath, NumerateFileName(strFilePath));
    }
    if (file.exists()) {
        if (file.size() == arrPic.size()) {
            LogOut("(disk) - Warning - file with same name and size exist \""
                   + file.fileName() + "\". Skipping...");
            return;
        }
        LogOut("(disk) - Warning - file with same name and DIFFERENT size"
               + QString("(exist '%1', new '%2') exist \"%3\". Numerate...")
               .arg(file.size()).arg(arrPic.size()).arg(file.fileName()));
        QString strNumFilePath = NumerateFileName(strFilePath);
        if (strNumFilePath.isEmpty()) {
            LogOut("(disk) - Error - numerate function return empty name.");
            return;
        }
        file.setFileName(strNumFilePath);

    }

    if (file.open(QIODevice::WriteOnly)) {
        if (file.write(arrPic) == arrPic.size()) {
            LogOut("(disk) - " + file.fileName() + " download success.");
            m_pFileSavedClbk->FileSaved(file.fileName());
        } else {
            LogOut("(disk) - Error - failed to write file \""
                   + file.fileName() + "\". Error string:"
                   + file.errorString());
        }
    } else {
        LogOut("(disk) - Error - failed to open file \""
               + file.fileName() + "\". Error string:"
               + file.errorString());
    }
}

// numerate file name unless file with this name does not exist
QString SerialPicsDownloader::NumerateFileName(const QString& strFileName)
{
    QString strNumFileName;
    for (int i = 1; i < 1000; i++) {
        strNumFileName = QString("%1_%2.jpeg").arg(strFileName).arg(i);
        QFile file(strNumFileName);
        if (!file.exists())
            return strNumFileName;
    }
    return "";
}
