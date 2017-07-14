// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "sqlitepicpreview.h"

#include <QtSql>
#include <QImageReader>
#include <chrono>

static const QString c_strDbFileName = "pi_collection_preview_cache.db";
static const QString c_strDbTableName = "cache_table";

static const QString c_strIdClmn =	"id";
static const QString c_strFileName = "FileName";
static const QString c_strFilePreview = "FilePreview";
static const QSize c_picPrevSize(1000, 180);

QSharedPointer<ISqLitePicPreview> ISqLitePicPreviewCtr(
        IMainLog *pLog, IWorkDir *pWorkDir)
{
    QSharedPointer<ISqLitePicPreview> retVal(
                new SqLitePicPreview(pLog, pWorkDir));
    return retVal;
}

bool SqLitePicPreview::GetPreview(const QString &strFile,
                                  QByteArray &retPreview)
{
    ReInitDBifPathChanged();
    ProcessReadyTasks();
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
    int itemsPerReq(10);
    QStringList lstFNameToQuery, lstFNamePreviewAbsent;
    for (int i = 1; itFrom >= m_itFNameInGenQueue; i++) {
        if (m_itFNameInGenQueue != nullptr && itFrom < m_itFNameInGenQueue) {
            break;			// element already in queue
        }
        lstFNameToQuery.push_back(*itFrom++);
        if (i % itemsPerReq == 0 || itFrom == lstFileNames.constEnd()) {
            lstFNamePreviewAbsent = GetAbsentFrom(lstFNameToQuery);
            if (lstFNamePreviewAbsent.size() * 2 > itemsPerReq) {
                // adjust items count per request
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
    while(ProcessReadyTasks() == 0) {
        // we need to wait at most 1 processed, because next call
        // to GetPreview expect first queued element will be ready
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(50ms);
    }
}

SqLitePicPreview::SqLitePicPreview(IMainLog *pLog, IWorkDir *pWorkDir)
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
    if (m_strWDPath != workDir){
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

void SqLitePicPreview::AddPreviewToDB(const QString & strFile,
                                      const QByteArray &bytePicPreview)
{
    QSqlQuery sqlQuery(*m_pSqLiteDB);
    QString strInsertCommand = QString(
                "INSERT INTO " + c_strDbTableName
                + "(" + c_strFileName + ", " + c_strFilePreview
                + ") VALUES('%1', :imageData)").arg(strFile);

    sqlQuery.prepare(strInsertCommand);
    sqlQuery.bindValue( ":imageData", bytePicPreview);
    if(!sqlQuery.exec()) {
        HandleFailedSqlQuery(sqlQuery.lastError().text());
        return;
    }
}

QByteArray SqLitePicPreview::GetPreviewFromDB(const QString &strFile)
{
    QByteArray retVal;
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
    *(strSelectCommand.end() - 1) = ')';	// replace last ',' on ')'

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

void SqLitePicPreview::EnqueueTasks(const QStringList &lstFileNames)
{
    {
        std::lock_guard<std::mutex> guard(m_mutexQueueGuard);
        for (const auto &item: lstFileNames) {
            m_deqFileNamesIn.push_back(item);
        }
    }
    StartAsyncPool();
}

int SqLitePicPreview::ProcessReadyTasks(int iMaxToProcess)
{
    int retVal(0);
    while (!m_deqPreviewsOut.empty()) {
        using namespace std;
        const auto item = m_deqPreviewsOut.front();
        if (*get<2>(item)) {
            if (!get<3>(item).isEmpty()) {
                LogOut(get<3>(item));	// resize task ends with error
                retVal++;
            } else if (!get<1>(item).isEmpty()) {
                AddPreviewToDB(get<0>(item), get<1>(item));
                retVal++;
            }
        } else {
            break;     // no more in ready state
        }
        {
            lock_guard<mutex> guard(m_mutexQueueGuard);
            m_deqPreviewsOut.pop_front();
        }
        if (retVal >= iMaxToProcess) {
            break;
        }
    }
    return retVal;
}

void SqLitePicPreview::StartAsyncPool()
{
    auto concur = std::thread::hardware_concurrency();
    if (concur == m_atomTaskCnt) {
        return;
    }
    if (m_vFutures.empty()) {
        for (unsigned int i = 0; i < concur; i++) {
            m_vFutures.push_back(std::async(std::launch::async,
                                            &SqLitePicPreview::AsyncResizeTask,
                                            this));
        }
    } else {
        for (unsigned int i = 0; i < concur; i++) {
            auto status = m_vFutures[i].wait_for(std::chrono::seconds(0));
            if (status == std::future_status::ready) {
                m_vFutures[i].get();
                m_vFutures[i] = std::async(std::launch::async,
                                           &SqLitePicPreview::AsyncResizeTask,
                                           this);
            }
        }
    }
}

void SqLitePicPreview::AsyncResizeTask() noexcept
{
    m_atomTaskCnt++;
    //QElapsedTimer timer; qint64 elapMutex;
    while (true) {
        using namespace std;
        previewItem *workItem(nullptr);
        {
            //timer.start();
            lock_guard<mutex> guard(m_mutexQueueGuard);
            //elapMutex = timer.elapsed();
            if (m_deqFileNamesIn.empty()) {
                break;
            }
            auto strFileName = m_deqFileNamesIn.front();
            m_deqFileNamesIn.pop_front();
            auto pAtmcReady = shared_ptr<atomic_bool>(new atomic_bool(false));
            m_deqPreviewsOut.push_back(previewItem(strFileName, QByteArray(),
                                                   pAtmcReady, QString()));
            workItem = &m_deqPreviewsOut.back();
        }
        //timer.start();
        get<3>(*workItem) = ResizeImage(m_strWDPath + get<0>(*workItem),
                                        get<1>(*workItem));
        //qDebug() << elapMutex << " and resize" << timer.elapsed();
        *get<2>(*workItem) = true;
    }
    m_atomTaskCnt--;
}

QString ResizeImage(const QString &strFilePath, QByteArray &retPreview)
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
