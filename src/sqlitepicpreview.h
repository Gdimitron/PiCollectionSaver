// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include "topleveiInterfaces.h"
#include <QtSql>

class SqLitePicPreview : public ISqLitePicPreview
{
    IMainLog *m_pLog;
    QString m_strFolder;

    QSqlDatabase m_sqLiteDB;
    const QString m_strTableName;

public:
    SqLitePicPreview(const QString &strDBFileName, IMainLog *pLog,
                     const QString &strFolder);
    ~SqLitePicPreview();
    QByteArray GetBase64Preview(const QString &strFile);

private:
    void LogOut(const QString &strMessage);
    bool IsFileNameExist(const QString& strFile);
    void AddPreviewToDB(const QString & strFile,
                        const QByteArray &bytePicPreview);
    QByteArray GetPreviewFromDB(const QString &strFile);
};
