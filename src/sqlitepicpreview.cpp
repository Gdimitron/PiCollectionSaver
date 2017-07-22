// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "sqlitepicpreview.h"

#include <QtSql>
#include <QImageReader>
#include <QLoggingCategory>
#include <chrono>

Q_LOGGING_CATEGORY(DBPreview, "sqlite.pic_preview", QtWarningMsg) // QtDebugMsg

static const QString c_strDbFileName = "pi_collection_preview_cache.db";
static const QString c_strDbTableName = "cache_table";

static const QString c_strIdClmn = "id";
static const QString c_strFileName = "FileName";
static const QString c_strFilePreview = "FilePreview";
static const QSize c_picPrevSize(1000, 180);

QSharedPointer<ISqLitePicPreview> ISqLitePicPreviewCtr(IMainLog *pLog,
                                                       IWorkDir *pWorkDir)
{
    QSharedPointer<ISqLitePicPreview> retVal(
                new SqLitePicPreview(pLog, pWorkDir));
    return retVal;
}

bool SqLitePicPreview::GetPreview(const QString &strFile,
                                  QByteArray &retPreview,
                                  bool bWaitReady)
{
    ReInitDBifPathChanged();
    if (bWaitReady) {
        while(!IsPreviewReady(strFile)) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(50ms);
        }
    }
    ProcessReady();
    retPreview = GetPreviewFromDB(strFile);
    if (retPreview.isEmpty()) {
        return false;
    }
    return true;
}

void SqLitePicPreview::AddNotExistRange(const QStringList &lstFileNames,
                                        QStringList::const_iterator itFrom)
{
    const int itemsPerReqMax(1000);
    auto itemsPerReq = static_cast<int>(m_tskCnt * m_tskBufItemCnt);
    QStringList lstFNameToQuery, lstFNamePreviewAbsent;
    for (int i = 1; itFrom >= m_itFNameInGenQueue; i++) {
        if (m_itFNameInGenQueue != nullptr && itFrom < m_itFNameInGenQueue) {
            break;			// element already in queue
        }
        lstFNameToQuery.push_back(*itFrom++);
        if (i % itemsPerReq == 0 || itFrom == lstFileNames.constEnd()) {
            lstFNamePreviewAbsent = GetAbsentFrom(lstFNameToQuery);
            if (lstFNamePreviewAbsent.size() * 2 > itemsPerReq) {
                // adjust items count per DB request (adjust window)
                itemsPerReq = std::min(itemsPerReq * 2, itemsPerReqMax);
            }
            if (lstFNamePreviewAbsent.empty()) {
                Q_ASSERT(i > 1);
                break;
            }
            EnqueueTasks(lstFNamePreviewAbsent);
            m_itFNameInGenQueue = itFrom;
            if (itFrom == lstFileNames.constEnd()) {
                break;
            }
            lstFNameToQuery.clear();
        }
    }
}

SqLitePicPreview::SqLitePicPreview(IMainLog *pLog,
                                   IWorkDir *pWorkDir)
    : m_pLog(pLog), m_pWorkDir(pWorkDir)
{
}

SqLitePicPreview::~SqLitePicPreview()
{
    if (m_pSqLiteDB) {
        m_pSqLiteDB->close();
    }
}

void SqLitePicPreview::InitPreviewDB()
{
    if (m_pSqLiteDB) {
        m_pSqLiteDB->close();
    }
    m_itFNameInGenQueue = nullptr;
    ClearDBPrevCache();

    auto dbPath = m_strWDPath + "/" + c_strDbFileName;
    m_pSqLiteDB = std::make_shared<QSqlDatabase>(
                QSqlDatabase::addDatabase("QSQLITE", "preview_cache_connect"));
    m_pSqLiteDB->setDatabaseName(dbPath);
    if (!m_pSqLiteDB->open()) {
        LogOut("Failed to open SqLite db(\"" + dbPath + "\"): "
               + m_pSqLiteDB->lastError().text());
        return;
    }
    QStringList lstTableList = m_pSqLiteDB->tables();
    if (lstTableList.indexOf(c_strDbTableName) == -1) {
        QString str =
                "CREATE TABLE " + c_strDbTableName + " ( "
                + c_strIdClmn + " INTEGER PRIMARY KEY ASC, "
                + c_strFileName + " TEXT, "
                + c_strFilePreview + " BLOB );";

        QSqlQuery sqlQuery(*m_pSqLiteDB);
        if(!sqlQuery.exec(str)) {
            QSqlError err = sqlQuery.lastError();
            LogOut("Failed to execute sql query: " + err.text());
            return;
        }
    }
}

void SqLitePicPreview::ReInitDBifPathChanged()
{
    auto workDir = m_pWorkDir->GetWD();
    if (m_strWDPath != workDir) {
        m_strWDPath = workDir;
        InitPreviewDB();
    }
}

void SqLitePicPreview::LogOut(const QString &strMessage)
{
    if (m_pLog) {
        m_pLog->LogOut("------[SqLitePicPreview] Error - " + strMessage);
    }
}

void SqLitePicPreview::HandleFailedSqlQuery(const QString &sqlErr)
{
    LogOut("Failed to execute sql query: " + sqlErr);
}

bool SqLitePicPreview::IsFileNameExist(const QString &strFile)
{
    QString strSelectCommand =
            "SELECT " + c_strFileName + " FROM " + c_strDbTableName
            + " WHERE " + c_strFileName + " IS " + strFile;

    QSqlQuery sqlQuery(*m_pSqLiteDB);
    if(!sqlQuery.exec(strSelectCommand)) {
        HandleFailedSqlQuery(sqlQuery.lastError().text());
        return false;
    }

    int iResultCount = 0;
    while (sqlQuery.next()) {
        iResultCount++;
    }
    Q_ASSERT(iResultCount < 2);
    return iResultCount > 0;
}

void SqLitePicPreview::DBAddPreviews(
        const std::vector<std::pair<QString, QByteArray>> vals)
{
    QSqlQuery sqlQuery(*m_pSqLiteDB);
    QString strInsertCommand = "INSERT INTO " + c_strDbTableName
                + "(" + c_strFileName + ", " + c_strFilePreview + ") VALUES ";

    int i = 0;
    for (const auto& item: vals) {
        strInsertCommand += QString("('%1', :img%2),").arg(item.first).arg(i++);
    }
    *(strInsertCommand.end() - 1) = ' ';	// replace last ','
    sqlQuery.prepare(strInsertCommand);

    i = 0;
    for (const auto& item: vals) {
        sqlQuery.bindValue(QString(":img%1").arg(i++), item.second);
        InsertInDBPrevCache(item.first, item.second);
    }
    if(!sqlQuery.exec()) {
        HandleFailedSqlQuery(sqlQuery.lastError().text());
        return;
    }
}

QByteArray SqLitePicPreview::GetPreviewFromDB(const QString &strFile)
{
    QByteArray retVal = GetFromDBPrevCache(strFile);
    if (!retVal.isEmpty()) {
        return retVal;
    }
    QString strSelectCommand =
            "SELECT " + c_strFilePreview + " FROM " + c_strDbTableName
            + " WHERE " + c_strFileName + " IS '" + strFile + "'";

    QSqlQuery sqlQuery(*m_pSqLiteDB);
    if(!sqlQuery.exec(strSelectCommand)) {
        HandleFailedSqlQuery(sqlQuery.lastError().text());
        return retVal;
    }
    if (sqlQuery.next()) {
        retVal = sqlQuery.value(0).toByteArray();
    }
    return retVal;
}

QStringList SqLitePicPreview::GetAbsentFrom(const QStringList &lstFileNames)
{
    // sqlite> select FileName from cache_table where FileName in (values
    //		('Wall06.jpg_.jpg'), ('2017-03-26_00-08.jpg'), ('not exist name'));
    // Wall06.jpg_.jpg
    // 2017-03-26_00-08.jpg
    // sqlite>
    QStringList retVal = lstFileNames;
    QString strSelectCommand =
            "SELECT " + c_strFileName + " FROM " + c_strDbTableName
            + " WHERE " + c_strFileName + " IN (VALUES ";
    for (const auto& item: lstFileNames) {
        strSelectCommand += "('" + item + "'),";
    }
    *(strSelectCommand.end() - 1) = ')';	// replace last ','

    QSqlQuery sqlQuery(*m_pSqLiteDB);
    if(!sqlQuery.exec(strSelectCommand)) {
        HandleFailedSqlQuery(sqlQuery.lastError().text());
        return retVal;
    }
    while (sqlQuery.next()) {
        retVal.removeOne(sqlQuery.value(0).toString());
    }
    return retVal;
}

void SqLitePicPreview::InsertInDBPrevCache(const QString &fName,
                                           const QByteArray &pic)
{
    m_DBPrevCache.insert(std::make_pair(fName, pic));
    if (DBPreview().isDebugEnabled()){
        static int dbg_cnt = 0;		// dump unordered_map stats
        if (dbg_cnt++ % 150 == 0) {
            qCDebug(DBPreview) << "m_DBPrevCache sz: " << m_DBPrevCache.size()
                 << " load_factor: " << m_DBPrevCache.load_factor()
                 << " bucket_count: " << m_DBPrevCache.bucket_count();

            std::unordered_map<unsigned long, int> bucket_distribution;
            for (unsigned int i = 0; i < m_DBPrevCache.bucket_count(); i++) {
                bucket_distribution[m_DBPrevCache.bucket_size(i)]++;
            }
            for (unsigned long i = 0; i < bucket_distribution.size(); i++) {
                qCDebug(DBPreview) << i << "element(s) in bucket: "
                                   << bucket_distribution[i];
            }
        }
    }
}

bool SqLitePicPreview::IsPreviewReady(const QString& strFileName) {
    std::lock_guard<std::mutex> guard(m_mutexQueueGuard);
    auto isInInputQueue = std::find(m_deqFNamesIn.begin(), m_deqFNamesIn.end(),
                                    strFileName) != m_deqFNamesIn.end();
    if (isInInputQueue) {
        return false;
    }
    auto lambdaExistAndReady = [&strFileName](const previewItem& item) {
        if (std::get<0>(item) == strFileName && *std::get<2>(item)) {
            return true;
        }
        return false;
    };
    auto ready = std::find_if(m_deqPreviewsOut.begin(), m_deqPreviewsOut.end(),
                              lambdaExistAndReady) != m_deqPreviewsOut.end();
    return ready;
}

void SqLitePicPreview::EnqueueTasks(const QStringList &lstFileNames)
{
    {
        std::lock_guard<std::mutex> guard(m_mutexQueueGuard);
        for (const auto &item: lstFileNames) {
            m_deqFNamesIn.push_back(item);
            m_atmItemInQueueCnt++;
        }
    }
    qCDebug(DBPreview) << "Enqueuened: " << lstFileNames.size();
    StartAsyncPool();
}

int SqLitePicPreview::ProcessReady(int iMaxToProcess)
{
    using namespace std;
    int processed(0);
    vector<pair<QString, QByteArray>> vAddToDB;
    {
        lock_guard<mutex> guard(m_mutexQueueGuard);
        while (!m_deqPreviewsOut.empty()) {
            const auto item = m_deqPreviewsOut.front();
            if (*get<2>(item)) { // resize task ends...
                if (!get<3>(item).isEmpty()) { // if ends with error
                    LogOut(get<3>(item));
                } else if (!get<1>(item).isEmpty()) { // success
                    vAddToDB.push_back(make_pair(get<0>(item), get<1>(item)));
                } else {
                    Q_ASSERT(false);
                }
                m_deqPreviewsOut.pop_front();
                processed++;
            } else {
                break;     // no more in 'ready' state
            }
            if (processed >= iMaxToProcess) {
                break;
            }
        }
    }
    if (processed) {
        QElapsedTimer tmr; if (DBPreview().isDebugEnabled()) { tmr.start(); }

        DBAddPreviews(vAddToDB);

        qCDebug(DBPreview) << "Items add to DB: " << vAddToDB.size()
                           << " DB save time: " << tmr.elapsed();
    }
    return processed;
}

void SqLitePicPreview::StartAsyncPool()
{
    if (m_tskCnt == m_atmTskCnt) {
        return;		// all tasks running
    }
    for (unsigned int i = 0; i < m_tskCnt; i++) {
        using namespace std;
        if (get<0>(m_vTsk[i]).valid()) {
            using namespace chrono_literals;
            auto st = get<0>(m_vTsk[i]).wait_for(0s);
            if (st == future_status::ready) {
                // get result of completed (new items are waiting in queue)
                get<0>(m_vTsk[i]).get();
            }
        }
        try {
            get<0>(m_vTsk[i]) = async(launch::async,
                                      &SqLitePicPreview::AsyncResizeTask,
                                      this, i);
            m_atmTskCnt++;
        } catch (std::exception &ex) {
            LogOut(ex.what());
            return;
        }
    }
}

void SqLitePicPreview::AsyncResizeTask(unsigned int taskNumber) noexcept
{
    using namespace std;
    int processed(0);
    qCDebug(DBPreview) << taskNumber << "] Async task start";
    while (true) {
        if (!get<1>(m_vTsk[taskNumber]).empty()) {
            // process item from task buffer
            auto workItem = get<1>(m_vTsk[taskNumber]).front();
            get<1>(m_vTsk[taskNumber]).pop_front();
            get<3>(*workItem) = ResizeImage(m_strWDPath + get<0>(*workItem),
                                            get<1>(*workItem));
            *get<2>(*workItem) = true;	// mark item as ready
            processed++;
            continue;
        }
        // task buffer is empty - fill it:
        lock_guard<mutex> guard(m_mutexQueueGuard);
        if (m_deqFNamesIn.empty()) {
            Q_ASSERT(m_atmItemInQueueCnt == 0);
            break;
        }
        for (unsigned int i = 0; i < m_tskBufItemCnt; i++) {
            auto strFileName = m_deqFNamesIn.front();
            m_deqFNamesIn.pop_front();
            m_atmItemInQueueCnt--;
            auto pAtmcReady = shared_ptr<atomic_bool>(new atomic_bool(false));
            m_deqPreviewsOut.push_back(previewItem(strFileName, QByteArray(),
                                                   pAtmcReady, QString()));
            get<1>(m_vTsk[taskNumber]).push_back(&m_deqPreviewsOut.back());
            if (m_deqFNamesIn.empty()) {
                break;
            }
        }
    }
    qCDebug(DBPreview) << taskNumber << "] Async task exit Items processed:"
                       << processed;
    Q_ASSERT(m_atmItemInQueueCnt == 0);
    m_atmTskCnt--;
}

QString ResizeImage(const QString &strFilePath,
                    QByteArray &retPreview)
{
    QImage img;
    QImageReader imgReader(strFilePath);
    if (!imgReader.read(&img)) {
        return QString("Failed to load image \"" + strFilePath + "\" Error: "
                       + imgReader.errorString());
    }
    auto imgScl = img.scaled(c_picPrevSize, Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    if (imgScl.isNull()) {
        return QString("Failed to scale image \"" + strFilePath + "\" Error: "
                       + imgReader.errorString());
    }
    AddBottomFrame(imgScl);
    QBuffer buffer(&retPreview);
    if (!buffer.open(QIODevice::WriteOnly)) {
        return QString("Failed to open output buffer for resized image");
    }
    if (!imgScl.save(&buffer, "JPG", 75)) {
        return QString("Failed to save preview image as jpg");
    }
    return QString();
}

void AddBottomFrame(QImage &imgScl)
{
    // Add white line(3 pixel width) to visually separate image in gallery
    // Couldn't find another solution. Appropriate html/css styles does not
    // work for image in QTextBrowser
    auto lineWidth = 3;
    QRgb* p = reinterpret_cast<QRgb*>(
                imgScl.scanLine(imgScl.height() - lineWidth));
    QColor white(Qt::white);
    for (auto i = 0; i < imgScl.width() * lineWidth; i++) {
        p[i] = white.rgb();
    }
}
