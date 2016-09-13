// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include <QString>

#include "topleveiInterfaces.h"

class FileSaver
{
    IMainLog *m_pLog;
    IFileSavedCallback *m_pFileSavedClbk;
public:
    FileSaver(IMainLog *pLog, IFileSavedCallback *pFileSavedClbk);
    void SaveContentToFile(QByteArray &arrPic, const QString &strFile,
                           bool bOverwrite);

private:
    void LogOut(const QString &strMessage);
    QString GuessExtByContent(QByteArray &arrPic);
    QString NumerateFileName(const QString &strFileName, const QString &strExt);
};
