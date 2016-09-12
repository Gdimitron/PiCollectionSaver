// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "sqlitepicpreview.h"

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

void SqLitePicPreview::GetBase64Preview(const QString &strFile,
                                              QByteArray &retPreview)
{
    ReInitDBifPathChanged();
    retPreview = GetPreviewFromDB(strFile);
    if (retPreview.isEmpty()) {
        QByteArray bytes;
        QBuffer buffer(&bytes);
        QImage img(m_strWDPath + "/" + strFile);
        buffer.open(QIODevice::WriteOnly);
        auto imgScl = img.scaled(c_picPrevSize, Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);

        // Add white line(3 pixel width) to visually separate image in gallery
        // Couldn't find another solution. Html/css styles does not work for
        // image in QTextBrowser
        auto lineWidth = 3;
        QRgb* p = reinterpret_cast<QRgb*>(
                    imgScl.scanLine(imgScl.height() - lineWidth));
        QColor white(Qt::white);
        for (auto i = 0; i < imgScl.width() * lineWidth; i++) {
            p[i] = white.rgb();
        }

        imgScl.save(&buffer, "JPG", 75);
        retPreview = bytes.toBase64();
        switch (m_cacheMode) {
        case modeSpeed:
            AddPreviewToDB(strFile, retPreview);
            break;
        case modeSize:
            AddPreviewToDB(strFile, bytes);
            break;
        }
    } else {
        switch (m_cacheMode) {
            case modeSpeed:
            break;
        case modeSize:
            retPreview = retPreview.toBase64();
            break;
        }
    }
}

SqLitePicPreview::SqLitePicPreview(IMainLog *pLog, IWorkDir *pWorkDir)
    : m_cacheMode(modeSize), m_pLog(pLog), m_pWorkDir(pWorkDir)
{
}

SqLitePicPreview::~SqLitePicPreview()
{
    m_sqLiteDB.close();
}

void SqLitePicPreview::InitDB()
{
    m_sqLiteDB.close();
    auto dbPath = m_strWDPath + "/" + c_strDbFileName;
    m_sqLiteDB = QSqlDatabase::addDatabase("QSQLITE", "preview_cache_connect");
    m_sqLiteDB.setDatabaseName(dbPath);
    if (!m_sqLiteDB.open()) {
        LogOut("Failed to open SqLite db(\"" + dbPath + "\"): "
               + m_sqLiteDB.lastError().text());
        return;
    }
    QStringList lstTableList = m_sqLiteDB.tables();
    if (lstTableList.indexOf(c_strDbTableName) == -1) {
        QString str =
                "CREATE TABLE " + c_strDbTableName + " ( "
                + c_strIdClmn + " INTEGER PRIMARY KEY ASC, "
                + c_strFileName + " TEXT, "
                + c_strFilePreview + " BLOB );";

        QSqlQuery sqlQuery(m_sqLiteDB);
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

    QSqlQuery sqlQuery(m_sqLiteDB);
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
    QSqlQuery sqlQuery(m_sqLiteDB);
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
            + " WHERE " + c_strFileName + " IS '" + strFile +"'";

    QSqlQuery sqlQuery(m_sqLiteDB);
    if(!sqlQuery.exec(strSelectCommand)) {
        LogOut("Failed to execute sql query: " + sqlQuery.lastError().text());
        return retVal;
    }
    if (sqlQuery.next()) {
        retVal = sqlQuery.value(0).toByteArray();
    }
    return retVal;
}
