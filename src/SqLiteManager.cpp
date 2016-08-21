// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "SqLiteManager.h"

const QString c_strIdClmn           		=	"id";
const QString c_strUserName         		=	"UserName";
const QString c_strUserId           		=	"UserId";
const QString c_strLstActvTime          	=	"LastActivityTime";
//const QString c_strLstActvTimeProcessed	=	"LastActivityTimeProcessed";
const QString c_strFavClmn              	=	"Favorite";


QSharedPointer<ISqLiteManager> ISqLiteManagerCtr(
        const QString &strDBFileName, const QString &strTableName)
{
    QSharedPointer<ISqLiteManager> retVal(new CSqLiteManager(strDBFileName,
                                                             strTableName));
    return retVal;
}

CSqLiteManager::CSqLiteManager(const QString &strDBFileName,
                               const QString &strTableName)
    :m_strTableName(strTableName), m_iReturnedIndex(0)
{
    m_sqLiteDB = QSqlDatabase::addDatabase("QSQLITE");
    m_sqLiteDB.setDatabaseName(strDBFileName);

    /*m_sqLiteDB.setUserName("elton");
    m_sqLiteDB.setHostName("epica");
    m_sqLiteDB.setPassword("password");*/

    if (!m_sqLiteDB.open()) {
        QSqlError err = m_sqLiteDB.lastError(); qDebug() << err.text();
        return;
    }

    QStringList lstTableList = m_sqLiteDB.tables();
    if (lstTableList.isEmpty()
            || lstTableList.indexOf(m_strTableName) == -1)
    {
        QString str = "CREATE TABLE ";
        str += m_strTableName + " ( "
                + c_strIdClmn + " INTEGER PRIMARY KEY ASC,"		// ROWID alias
                + c_strUserName + " TEXT, "
                + c_strUserId + " TEXT, "
                + c_strLstActvTime + " TEXT,"
                //+ c_strLstActvTimeProcessed + " TEXT,"
                + c_strFavClmn + " INTEGER"
                ");";

        QSqlQuery sqlQuery;
        if(!sqlQuery.exec(str)) {
            QSqlError err = sqlQuery.lastError(); qDebug() << err.text();
            return;
        }
    }
}


CSqLiteManager::~CSqLiteManager(void)
{
}

QSqlDatabase &CSqLiteManager::getDb()
{
    return m_sqLiteDB;
}

bool CSqLiteManager::IsUserWithIdExist(const QString& strUserId)
{
    QString strSelectCommand = QString("SELECT * FROM " + m_strTableName
                                       + " WHERE " + c_strUserId
                                       + " IS " + strUserId);

    QSqlQuery sqlQuery;
    if(!sqlQuery.exec(strSelectCommand)) {
        QSqlError err = sqlQuery.lastError(); qDebug() << err.text();
        return false;
    }

    // I have not found easy way to determine the number of row
    // returned(sqlQuery.size() and row affected does not work ), so enumerate:
    int iResultCound = 0;
    while (sqlQuery.next()) {
        iResultCound++;
    }
    Q_ASSERT(iResultCound < 2);
    return iResultCound > 0;
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

    QString strInsertCommand
            = QString("INSERT INTO " + m_strTableName
                      + "(" + c_strUserName + ", " + c_strUserId + ", "
                      + c_strLstActvTime + ", " + c_strFavClmn
                      + ") VALUES('%1', '%2', '%3', 0);")
            .arg(strUserName).arg(strUserId).arg(strLstActvTime);

    QSqlQuery sqlQuery;
    if(!sqlQuery.exec(strInsertCommand)) {
        QSqlError err = sqlQuery.lastError(); qDebug() << err.text();
        return;
    }
}


qListPairOf2Str CSqLiteManager::GetFirstAllUsersIdActivityTime(
        int iCountMax, bool bFavoriteOnly, bool bEmptyActivityTimeOnly)
{
    m_iReturnedIndex = 0;
    return GetAllUsersIdActivityTimeImpl(iCountMax, bFavoriteOnly,
                                         bEmptyActivityTimeOnly);
}


qListPairOf2Str CSqLiteManager::GetNextAllUsersIdActivityTime(
        int iCountMax, bool bFavoriteOnly, bool bEmptyActivityTimeOnly)
{
    return GetAllUsersIdActivityTimeImpl(iCountMax, bFavoriteOnly,
                                         bEmptyActivityTimeOnly);
}


qListPairOf2Str CSqLiteManager::GetAllUsersIdActivityTimeImpl(
        int iCountMax, bool bFavoriteOnly, bool bEmptyActivityTimeOnly)
{
    qListPairOf2Str lstReturn;
    QPair<QString, QString> prRow;

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

        QSqlQuery sqlQuery;
        if(!sqlQuery.exec(strSelectCommand) || !sqlQuery.next()) {
            QSqlError err = sqlQuery.lastError(); qDebug() << err.text();
            return lstReturn;
        }

        QSqlRecord sqlRec(sqlQuery.record());
        do {
            m_iReturnedIndex++;

            if (bFavoriteOnly && sqlQuery.value(
                        sqlRec.indexOf(c_strFavClmn)).toInt() == 0) {
                continue;
            }
            if (bEmptyActivityTimeOnly && sqlQuery.value(
                        sqlRec.indexOf(c_strLstActvTime)).toString() != "") {
                continue;
            }

            prRow.first = sqlQuery.value(
                        sqlRec.indexOf(c_strUserId)).toString();
            prRow.second = sqlQuery.value(
                        sqlRec.indexOf(c_strLstActvTime)).toString();
            lstReturn.append(prRow);
            iTotalSelectCount++;

            int iRecId = sqlQuery.value(sqlRec.indexOf(c_strIdClmn)).toInt();
            if (iRecId == iMaxTableId) {
                bEndReached = true;
            }
        } while (sqlQuery.next());
    }
    return lstReturn;
}

int CSqLiteManager::GetMaxTableId()
{
    QString strSelectCommand = "SELECT MAX(" + c_strIdClmn +
            ") AS LargestId FROM " + m_strTableName + " p1";
    QSqlQuery sqlQuery;
    if(!sqlQuery.exec(strSelectCommand) || !sqlQuery.next()) {
        QSqlError err = sqlQuery.lastError(); qDebug() << err.text();
        return -1;
    }
    return sqlQuery.value(sqlQuery.record().indexOf("LargestId")).toInt();
}

void CSqLiteManager::UpdateLastActivityTime(
        QPair<QString, QString> pairUserNameLastActivityTime)
{
    QString strUpdateCommand = QString("UPDATE " + m_strTableName + " SET "
                                       + c_strLstActvTime + " = '%2' WHERE "
                                       + c_strUserId + " = '%1'")
            .arg(pairUserNameLastActivityTime.first)
            .arg(pairUserNameLastActivityTime.second);

    QSqlQuery sqlQuery;
    if(!sqlQuery.exec(strUpdateCommand)) {
        QSqlError err = sqlQuery.lastError(); qDebug() << err.text();
        return;
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

//        QSqlQuery sqlQuery;
//        if(!sqlQuery.exec(strUpdateCommand)) {
//            QSqlError err = sqlQuery.lastError(); qDebug() << err.text();
//            return;
//        }
//    }
//}
