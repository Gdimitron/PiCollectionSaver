// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "sqlitepicpreview.h"

static const QString c_strIdClmn =	"id";
static const QString c_strFileName = "FileName";
static const QString c_strFilePreview = "FilePreview";
static const QSize c_picPrevSize(400, 400); //TODO: Fixed this (need to respect
                                            // aspect ratio to better view)

QSharedPointer<ISqLitePicPreview> ISqLitePicPreviewCtr(
        const QString &strDBFileName, IMainLog *pLog, const QString &strFolder)
{
    QSharedPointer<ISqLitePicPreview> retVal(
                new SqLitePicPreview(strDBFileName, pLog, strFolder));
    return retVal;
}

SqLitePicPreview::SqLitePicPreview(const QString &strDBFileName, IMainLog *pLog,
                                   const QString &strFolder)
    : m_pLog(pLog), m_strFolder(strFolder), m_strTableName("cache_table")
{
    m_sqLiteDB = QSqlDatabase::addDatabase("QSQLITE", "preview_cache_connect");
    m_sqLiteDB.setDatabaseName(strDBFileName);
    if (!m_sqLiteDB.open()) {
        LogOut("Failed to open SqLite db: " + m_sqLiteDB.lastError().text());
        return;
    }
    QStringList lstTableList = m_sqLiteDB.tables();
    if (lstTableList.isEmpty() || lstTableList.indexOf(m_strTableName) == -1) {
        QString str = "CREATE TABLE ";
        str += m_strTableName + " ( "
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

SqLitePicPreview::~SqLitePicPreview()
{
    m_sqLiteDB.close();
}

QByteArray SqLitePicPreview::GetBase64Preview(const QString &strFile)
{
    QByteArray retVal = GetPreviewFromDB(strFile);;
    if (retVal.isEmpty()) {
        QByteArray bytes;
        QBuffer buffer(&bytes);
        QImage img(m_strFolder + "/" + strFile);
        buffer.open(QIODevice::WriteOnly);
        img.scaled(c_picPrevSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                .save(&buffer, "JPG");
        AddPreviewToDB(strFile, bytes);
        retVal = bytes.toBase64();
    } else {
        retVal = retVal.toBase64();
    }
    return retVal;
}


void SqLitePicPreview::LogOut(const QString &strMessage)
{
    if (m_pLog) {
        m_pLog->LogOut("------[SqLitePicPreview] Error - " + strMessage);
    }
}

bool SqLitePicPreview::IsFileNameExist(const QString &strFile)
{
    QString strSelectCommand = QString("SELECT * FROM " + m_strTableName
                                       + " WHERE " + c_strFileName
                                       + " IS " + strFile);

    QSqlQuery sqlQuery(m_sqLiteDB);
    if(!sqlQuery.exec(strSelectCommand)) {
        LogOut("Failed to execute sql query: " + sqlQuery.lastError().text());
        return false;
    }

    // check sqlQuery.size(); and rowAffected, perhaps they works now

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
                "INSERT INTO " + m_strTableName
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
    QString strSelectCommand = QString("SELECT " + c_strFilePreview + " FROM "
                                       + m_strTableName
                                       + " WHERE " + c_strFileName
                                       + " IS '" + strFile +"'");

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
