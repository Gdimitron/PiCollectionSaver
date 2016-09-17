// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include "topleveiInterfaces.h"

class QSqlDatabase;

class SqLitePicPreview : public ISqLitePicPreview
{
    std::shared_ptr<QSqlDatabase> m_pSqLiteDB;
    IMainLog *m_pLog;
    IWorkDir *m_pWorkDir;
    QString m_strWDPath;

public:
    void GetPreview(const QString &strFile, QByteArray &retPreview);
    SqLitePicPreview(IMainLog *pLog, IWorkDir *pWorkDir);
    ~SqLitePicPreview();

private:
    void InitDB();
    void ReInitDBifPathChanged();
    void LogOut(const QString &strMessage);
    bool IsFileNameExist(const QString& strFile);
    void AddPreviewToDB(const QString & strFile,
                        const QByteArray &bytePicPreview);
    QByteArray GetPreviewFromDB(const QString &strFile);
};
