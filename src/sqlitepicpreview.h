// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include "topleveiInterfaces.h"

#include <future>
#include <queue>
#include <unordered_map>

class QSqlDatabase;
class SqLitePicPreview : public ISqLitePicPreview
{
    IMainLog *m_pLog;
    IWorkDir *m_pWorkDir;
    QString m_strWDPath;

    std::shared_ptr<QSqlDatabase> m_pSqLiteDB;
    std::function<std::size_t(const QString& str)> hashFunc {
        [] (const QString& str) { return qHash(str); }
    };
    std::unordered_map<QString, QByteArray, decltype(hashFunc)> m_DBPrevCache {
        300, hashFunc
    };

    std::mutex m_mutexQueueGuard;
    std::deque<QString> m_deqFNamesIn;
    QStringList::const_iterator m_itFNameInGenQueue { nullptr };

    using previewItem = std::tuple<
        QString,							// pic file name
        QByteArray,							// preview
        std::shared_ptr<std::atomic_bool>,	// ready flag
        QString>;							// error message
    std::deque<previewItem> m_deqPreviewsOut;

    const unsigned int m_tskCnt { std::thread::hardware_concurrency() };
    const unsigned int m_tskBufItemCnt { std::thread::hardware_concurrency() };
    using taskItem = std::tuple<
        std::future<void>,					// task future
        std::deque<previewItem*> >;			// task queue
    std::vector<taskItem> m_vTsk { std::vector<taskItem>::size_type(m_tskCnt) };
    std::atomic_uint m_atmTskCnt { 0 };
    std::atomic_uint m_atmItemInQueueCnt { 0 };

public:
    bool GetPreview(const QString &strFile,
                    QByteArray &retPreview,
                    bool bWaitReady = false);

    void AddNotExistRange(const QStringList &lstFileNames,
                          QStringList::const_iterator itFrom);

    SqLitePicPreview(IMainLog *pLog,
                     IWorkDir *pWorkDir);

    ~SqLitePicPreview();

private:
    void InitPreviewDB();
    void ReInitDBifPathChanged();
    void LogOut(const QString &strMessage);
    void HandleFailedSqlQuery(const QString &sqlErr);
    bool IsFileNameExist(const QString& strFile);
    void DBAddPreviews(const std::vector<std::pair<QString, QByteArray>> vals);
    QByteArray GetPreviewFromDB(const QString &strFile);
    QStringList GetAbsentFrom(const QStringList &lstFileNames);

    void InsertInDBPrevCache(const QString& fName,
                             const QByteArray& pic) {
        m_DBPrevCache.insert(std::make_pair(fName, pic));
    }
    QByteArray GetFromDBPrevCache(const QString& fName) {
        auto it = m_DBPrevCache.find(fName);
        if (it != m_DBPrevCache.end()) { return (*it).second; }
        return QByteArray();
    }
    void ClearDBPrevCache() {
        m_DBPrevCache.clear();
    }

    bool IsPreviewReady(const QString& strFileName);
    void EnqueueTasks(const QStringList &lstFileNames);
    int ProcessReady(int iMaxToProcess = 50);
    void StartAsyncPool();
    void AsyncResizeTask(unsigned int taskNumber) noexcept;
};

// support function
QString ResizeImage(const QString &strFilePath,
                    QByteArray &retPreview);
void AddBottomFrame(QImage &imgScl);
