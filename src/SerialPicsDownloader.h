// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include <QMutex>
#include <QThread>
#include <QDebug>
#include <QSharedPointer>

#include "topleveiInterfaces.h"
#include "HttpDownloader.h"
#include "filesaver.h"

struct stPicElement
{
    int m_iPicOrderNumber;  // from 0 to 1, 2, 3, and so on; Forbidden to save
    // picture with next order number, if previous not
    // saved yet;
    QString m_strDirectPicUrl;
    QString m_strPicPageUrl;
    QString m_strFileNameToSave;
    bool m_bDownloading;
    int m_iHttpReplyStatusCode;
    std::shared_ptr<IHtmlPageElm> m_ptrHtmlElmtPicPage;
    QByteArray m_arrPicContent;
    bool m_bDownloadDone;
    bool m_bDonwloadError;
    QString m_strError;
    bool m_bPicForBrowser;

    stPicElement(int iPicOrderNumber):	m_iPicOrderNumber(iPicOrderNumber),
        m_bDownloading(false), m_iHttpReplyStatusCode(0),
        m_bDownloadDone(false), m_bDonwloadError(false), m_bPicForBrowser(false)
    { }
};

class SerialPicsDownloader: public QObject, public ISerialPicsDownloader
{
    Q_OBJECT

public:
    SerialPicsDownloader(QObject *parent, IMainLog *pLog,
                         IFileSavedCallback *pFileSaved);
    virtual ~SerialPicsDownloader(void);

    void SetSite(ISite *pSite);
    void SetOverwriteMode(bool bOverwrite);
    void SetDestinationFolder(const QString& strDstFolderPath);
    void AddPicPageUrlLstToQueue(
            const QMap<QString, QString> &mapPicPageUrlFileName);

    QMap<QString, QString> GetAndClearErrPicsList();

signals:
    void QueueEmpty();

private slots:
    void httpSlotError(const QUrl&, const QString&);
    void httpSlotDone(int, const QUrl&, const QByteArray&);

private:
    HttpDownloader* m_pHttp;
    QList<stPicElement> m_lstMainPicsList;
    QList<stPicElement> m_lstMainPicsErrList;
    QMutex m_mutexForList;

    FileSaver m_fs;
    bool m_bOverwrite;
    QString m_strDestinationPath;

    ISite *m_pSite;
    IMainLog *m_pLog;

    int m_iCurrentMaxOrderNumber;

private: // methods
    void LogOut(const QString &strMessage);

    void AddElementToList(const QString &strPicPageUrl,
                          const QString &strFileNameToSave);

    void ListDownloadProcessingImpl(void);
    void PicPageDownloadDoneProcess(const QString &strDirectPicUrl,
                                    const QByteArray& arrPic,
                                    int iHttpReplyCode);
    void DirectPicDownloadDoneProcess(const QString &strDirectPicUrl,
                                      const QByteArray& arrPic,
                                      int iHttpReplyCode);
    void ListDownloadErrorProcess(const QString &strDirectPicUrl,
                                  const QString &strError);
    void ProcessDoneItems();
};
