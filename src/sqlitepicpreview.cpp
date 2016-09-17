// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "sqlitepicpreview.h"

#include <QtSql>
#include <QImageReader>

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

void SqLitePicPreview::GetPreview(const QString &strFile,
                                  QByteArray &retPreview)
{
    ReInitDBifPathChanged();
    retPreview = GetPreviewFromDB(strFile);
    if (retPreview.isEmpty()) {
        QImage img;
        QImageReader imgReader(m_strWDPath + "/" + strFile);
        if (!imgReader.read(&img)) {
            LogOut("Failed to load image \"" + m_strWDPath + "/" + strFile
                   + "\" Error: " + imgReader.errorString());
        }
        auto imgScl = img.scaled(c_picPrevSize, Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);
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
        QBuffer buffer(&retPreview);
        buffer.open(QIODevice::WriteOnly);
        imgScl.save(&buffer, "JPG", 75);
        AddPreviewToDB(strFile, retPreview);
    }
}

SqLitePicPreview::SqLitePicPreview(IMainLog *pLog, IWorkDir *pWorkDir)
    : m_pLog(pLog), m_pWorkDir(pWorkDir)
{
}

SqLitePicPreview::~SqLitePicPreview()
{
    m_pSqLiteDB->close();
}

void SqLitePicPreview::InitDB()
{
    if (m_pSqLiteDB) {
        m_pSqLiteDB->close();
    }
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
        InitDB();
    }
}

void SqLitePicPreview::LogOut(const QString &strMessage)
{
    if (m_pLog) {
        m_pLog->LogOut("------[SqLitePicPreview] Error - " + strMessage);
    }
}

bool SqLitePicPreview::IsFileNameExist(const QString &strFile)
{
    QString strSelectCommand =
            "SELECT * FROM " + c_strDbTableName
            + " WHERE " + c_strFileName + " IS " + strFile;

    QSqlQuery sqlQuery(*m_pSqLiteDB);
    if(!sqlQuery.exec(strSelectCommand)) {
        LogOut("Failed to execute sql query: " + sqlQuery.lastError().text());
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
        LogOut("Failed to execute sql query: " + sqlQuery.lastError().text());
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
        LogOut("Failed to execute sql query: " + sqlQuery.lastError().text());
        return retVal;
    }
    if (sqlQuery.next()) {
        retVal = sqlQuery.value(0).toByteArray();
    }
    return retVal;
}
