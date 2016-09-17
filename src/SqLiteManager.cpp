// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "SqLiteManager.h"

#include <QtSql>

const QString c_strIdClmn           		=	"id";
const QString c_strUserName         		=	"UserName";
const QString c_strUserId           		=	"UserId";
const QString c_strLstActvTime          	=	"LastActivityTime";
//const QString c_strLstActvTimeProcessed	=	"LastActivityTimeProcessed";
const QString c_strFavClmn              	=	"Favorite";

QSharedPointer<ISqLiteManager> ISqLiteManagerCtr(
        const QString &strDBFileName, const QString &strTableName,
        IMainLog *pLog)
{
    QSharedPointer<ISqLiteManager> retVal(
                new CSqLiteManager(strDBFileName, strTableName, pLog));
    return retVal;
}

CSqLiteManager::CSqLiteManager(const QString &strDBFileName,
                               const QString &strTableName, IMainLog *pLog)
    : m_strTableName(strTableName), m_pLog(pLog), m_iReturnedIndex(0)
{
    m_pSqLiteDB = std::make_shared<QSqlDatabase>(
                QSqlDatabase::addDatabase("QSQLITE", "users_connection"));
    m_pSqLiteDB->setDatabaseName(strDBFileName);
    if (!m_pSqLiteDB->open()) {
        LogOut("Failed to open SqLite db(\"" + strDBFileName + "\"): "
               + m_pSqLiteDB->lastError().text());
        return;
    }
    
    QStringList lstTableList = m_pSqLiteDB->tables();
    if (lstTableList.isEmpty() || lstTableList.indexOf(m_strTableName) == -1) {
        QString str = "CREATE TABLE " + m_strTableName + " ( "
                + c_strIdClmn + " INTEGER PRIMARY KEY ASC,"		// ROWID alias
                + c_strUserName + " TEXT, "
                + c_strUserId + " TEXT, "
                + c_strLstActvTime + " TEXT,"
                //+ c_strLstActvTimeProcessed + " TEXT,"
                + c_strFavClmn + " INTEGER"
                                 ");";
        
        QSqlQuery sqlQuery(*m_pSqLiteDB);
        if(!sqlQuery.exec(str)) {
            LogOut("Failed to execute sql query: "
                   + sqlQuery.lastError().text());
            return;
        }
    }
}

CSqLiteManager::~CSqLiteManager(void)
{
    m_pSqLiteDB->close();
}

QSqlDatabase *CSqLiteManager::getDb()
{
    return m_pSqLiteDB.get();
}

bool CSqLiteManager::IsUserWithIdExist(const QString& strUserId)
{
    QString strSelectCommand = QString("SELECT * FROM " + m_strTableName
                                       + " WHERE " + c_strUserId
                                       + " IS '" + strUserId + "'");
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

int CSqLiteManager::GetUserCnt()
{
    return GetMaxTableId();
}

void CSqLiteManager::AddNewUser(const QString & strUserName,
                                const QString & strUserId,
                                const QString & strLstActvTime)
{
    if (IsUserWithIdExist(strUserId)) {
        return;
    }

    QString strInsertCommand = QString(
                "INSERT INTO " + m_strTableName
                + "(" + c_strUserName + ", " + c_strUserId + ", "
                + c_strLstActvTime + ", " + c_strFavClmn
                + ") VALUES('%1', '%2', '%3', 0);")
            .arg(strUserName).arg(strUserId).arg(strLstActvTime);

    QSqlQuery sqlQuery(*m_pSqLiteDB);
    if(!sqlQuery.exec(strInsertCommand)) {
        LogOut("Failed to execute sql query: " + sqlQuery.lastError().text());
        return;
    }
}


QMap<QString, QString> CSqLiteManager::GetFirstAllUsersIdActivityTime(
        int iCountMax, bool bFavoriteOnly, bool bEmptyActivityTimeOnly)
{
    m_iReturnedIndex = 0;
    return GetAllUsersIdActivityTimeImpl(iCountMax, bFavoriteOnly,
                                         bEmptyActivityTimeOnly);
}


QMap<QString, QString> CSqLiteManager::GetNextAllUsersIdActivityTime(
        int iCountMax, bool bFavoriteOnly, bool bEmptyActivityTimeOnly)
{
    return GetAllUsersIdActivityTimeImpl(iCountMax, bFavoriteOnly,
                                         bEmptyActivityTimeOnly);
}


QMap<QString, QString> CSqLiteManager::GetAllUsersIdActivityTimeImpl(
        int iCountMax, bool bFavoriteOnly, bool bEmptyActivityTimeOnly)
{
    QMap<QString, QString> mapRes;

    int iTotalSelectCount(0);
    bool bEndReached(false);
    int iMaxTableId = GetMaxTableId();

    for (int iIdIndexInc = iCountMax;
         iTotalSelectCount < iCountMax && !bEndReached;
         iIdIndexInc = iCountMax - iTotalSelectCount)
    {
        Q_ASSERT(iIdIndexInc > 0);
        QString strSelectCommand
                = QString("SELECT * FROM " + m_strTableName +
                          " WHERE " + c_strIdClmn + " BETWEEN %1 AND %2")
                .arg(m_iReturnedIndex + 1).arg(m_iReturnedIndex + iIdIndexInc);

        QSqlQuery sqlQuery(*m_pSqLiteDB);
        if(!sqlQuery.exec(strSelectCommand)) {
            LogOut("Failed to execute sql query: "
                   + sqlQuery.lastError().text());
            return mapRes;
        }

        QSqlRecord sqlRec(sqlQuery.record());
        while (sqlQuery.next()) {
            m_iReturnedIndex++;

            if (bFavoriteOnly && sqlQuery.value(
                        sqlRec.indexOf(c_strFavClmn)).toInt() == 0) {
                continue;
            }
            if (bEmptyActivityTimeOnly && sqlQuery.value(
                        sqlRec.indexOf(c_strLstActvTime)).toString() != "") {
                continue;
            }

            auto userId = sqlQuery.value(
                        sqlRec.indexOf(c_strUserId)).toString();
            auto activeTime = sqlQuery.value(
                        sqlRec.indexOf(c_strLstActvTime)).toString();
            mapRes[userId] = activeTime;
            iTotalSelectCount++;

            int iRecId = sqlQuery.value(sqlRec.indexOf(c_strIdClmn)).toInt();
            if (iRecId == iMaxTableId) {
                bEndReached = true;
            }
        }
        if (m_iReturnedIndex >= iMaxTableId) {
            bEndReached = true;
        }
    }
    return mapRes;
}

int CSqLiteManager::GetMaxTableId()
{
    QString strSelectCommand = "SELECT MAX(" + c_strIdClmn +
            ") AS LargestId FROM " + m_strTableName + " p1";
    QSqlQuery sqlQuery(*m_pSqLiteDB);
    if(!sqlQuery.exec(strSelectCommand)) {
        LogOut("Failed to execute sql query: " + sqlQuery.lastError().text());
        return -1;
    }
    if (sqlQuery.next()) {
        return sqlQuery.value(sqlQuery.record().indexOf("LargestId")).toInt();
    }
    return -1;
}

void CSqLiteManager::UpdateLastActivityTime(const QString &strName,
                                            const QString &strActivTime)
{
    QString strUpdateCommand = QString(
                "UPDATE " + m_strTableName + " SET "
                + c_strLstActvTime + " = '%2' WHERE "
                + c_strUserId + " = '%1'").arg(strName).arg(strActivTime);

    QSqlQuery sqlQuery(*m_pSqLiteDB);
    if(!sqlQuery.exec(strUpdateCommand)) {
        LogOut("Failed to execute sql query: " + sqlQuery.lastError().text());
    }
}

void CSqLiteManager::LogOut(const QString &strMessage)
{
    if (m_pLog) {
        m_pLog->LogOut("------[SqLiteManager] Error - " + strMessage);
    }
}

//void CSqLiteManager::UpdateLastActivityTimeProcessed(qListPairOf2Str lstUserIdTime)
//{
//    QPair<QString, QString> pairUserNameLastActivityTime;
//    foreach(pairUserNameLastActivityTime, lstUserIdTime) {
//        QString strUpdateCommand = QString("UPDATE " + m_strTableName + " SET "
//            + c_strLstActvTimeProcessed + " = '%2' WHERE " + c_strUserId + " = '%1'")
//            .arg(pairUserNameLastActivityTime.first)
//            .arg(pairUserNameLastActivityTime.second);

//        QSqlQuery sqlQuery(*m_pSqLiteDB);
//        if(!sqlQuery.exec(strUpdateCommand)) {
//            QSqlError err = sqlQuery.lastError(); qDebug() << err.text();
//            return;
//        }
//    }
//}
