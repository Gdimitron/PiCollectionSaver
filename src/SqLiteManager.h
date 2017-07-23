// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include "topleveiInterfaces.h"

class QSqlDatabase;

class CSqLiteManager : public ISqLiteManager
{
    std::shared_ptr<QSqlDatabase> m_pSqLiteDB;
    QString m_strTableName;
    IMainLog *m_pLog;

    int m_iReturnedIndex;

public:
    CSqLiteManager(const QString &strDBFileName,
                   const QString &strTableName,
                   IMainLog *pLog);
    virtual ~CSqLiteManager(void);

    QSqlDatabase *getDb();

    bool IsUserWithIdExist(const QString& strUserId);

    int GetUserCnt();

    void AddNewUser(const QString & strUserName,     // format: "user66"
                    const QString & strUserId,       // format: "117235"
                    const QString & strLstActvTime); // - "11/07/2012 10:46:01"

    QMap<QString, QString> GetFirstAllUsersIdActivityTime(
            int iCountMax,
            bool bFavoriteOnly = false,
            bool bEmptyActivityTimeOnly = false);

    QMap<QString, QString> GetNextAllUsersIdActivityTime(
            int iCountMax,
            bool bFavoriteOnly = false,
            bool bEmptyActivityTimeOnly = false);

    void UpdateLastActivityTime(const QString &strName,
                                const QString &strActivTime);
    //void UpdateLastActivityTimeProcessed(qListPairOf2Str lstUserIdTime);

private:
    void LogOut(const QString &strMessage);

    QMap<QString, QString> GetAllUsersIdActivityTimeImpl(
            int iCountMax,
            bool bFavoriteOnly = false,
            bool bEmptyActivityTimeOnly = false);

    int GetMaxTableId();
};
