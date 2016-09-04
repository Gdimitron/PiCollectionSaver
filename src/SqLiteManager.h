// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include "topleveiInterfaces.h"
#include <QtSql>

class CSqLiteManager : public ISqLiteManager
{
    QSqlDatabase m_sqLiteDB;
    QString m_strTableName;
    IMainLog *m_pLog;

    int m_iReturnedIndex;

public:
    CSqLiteManager(const QString &strDBFileName, const QString &strTableName,
                   IMainLog *pLog);
    virtual ~CSqLiteManager(void);

    QSqlDatabase &getDb();

    bool IsUserWithIdExist(const QString& strUserId);   // format: "117235"

    int GetUserCnt();

    void AddNewUser(const QString & strUserName,     // format: "user66"
                    const QString & strUserId,       // format: "117235"
                    const QString & strLstActvTime); // - "11/07/2012 10:46:01"

    qListPairOf2Str GetFirstAllUsersIdActivityTime(int iCountMax,
            bool bFavoriteOnly = false, bool bEmptyActivityTimeOnly = false);

    qListPairOf2Str GetNextAllUsersIdActivityTime(int iCountMax,
            bool bFavoriteOnly = false, bool bEmptyActivityTimeOnly = false);

    void UpdateLastActivityTime(
            QPair<QString, QString> pairUserNameLastActivityTime);
    //void UpdateLastActivityTimeProcessed(qListPairOf2Str lstUserIdTime);

private:
    void LogOut(const QString &strMessage);

    qListPairOf2Str GetAllUsersIdActivityTimeImpl(int iCountMax,
            bool bFavoriteOnly = false, bool bEmptyActivityTimeOnly = false);
    int GetMaxTableId();
};
