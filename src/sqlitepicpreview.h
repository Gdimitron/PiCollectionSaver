// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include "topleveiInterfaces.h"

#include <future>
#include <queue>

class QSqlDatabase;

class SqLitePicPreview : public ISqLitePicPreview
{
    std::shared_ptr<QSqlDatabase> m_pSqLiteDB;
    IMainLog *m_pLog;
    IWorkDir *m_pWorkDir;
    QString m_strWDPath;

    std::vector<std::future<void>> m_vFutures;
    std::atomic_uint m_atomTaskCnt { 0 };

    std::mutex m_mutexQueueGuard;
    std::deque<QString> m_deqFileNamesIn;
    QStringList::const_iterator m_itFNameInGenQueue { nullptr };
    using previewItem = std::tuple<QString, QByteArray,
                                    std::shared_ptr<std::atomic_bool>, QString>;
    std::deque<previewItem> m_deqPreviewsOut;

public:
    bool GetPreview(const QString &strFile, QByteArray &retPreview);
    void AddNotExistRange(const QStringList &lstFileNames,
                          QStringList::const_iterator itFrom);
    SqLitePicPreview(IMainLog *pLog, IWorkDir *pWorkDir);
    ~SqLitePicPreview();

private:
    void InitPreviewDB();
    void ReInitDBifPathChanged();
    void LogOut(const QString &strMessage);
    void HandleFailedSqlQuery(const QString &sqlErr);
    bool IsFileNameExist(const QString& strFile);
    void AddPreviewToDB(const QString & strFile,
                        const QByteArray &bytePicPreview);
    QByteArray GetPreviewFromDB(const QString &strFile);
    QStringList GetAbsentFrom(const QStringList &lstFileNames);

    void EnqueueTasks(const QStringList &lstFileNames);
    int ProcessReadyTasks(int iMaxToProcess = 10);
    void StartAsyncPool();
    void AsyncResizeTask() noexcept;
};

// support function
QString ResizeImage(const QString &strFilePath, QByteArray &retPreview);
void AddBottomFrame(QImage &imgScl);
