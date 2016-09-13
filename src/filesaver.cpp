// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "filesaver.h"

#include <QImageReader>
#include <QBuffer>
#include <QFile>

FileSaver::FileSaver(IMainLog *pLog, IFileSavedCallback *pFileSavedClbk)
    : m_pLog(pLog), m_pFileSavedClbk(pFileSavedClbk)
{

}

void FileSaver::SaveContentToFile(QByteArray& arrPic, const QString& strFile,
                                  bool bOverwrite)
{
    auto strExt = GuessExtByContent(arrPic);
    QFile file(strFile + "." + strExt);
    if (file.exists() && bOverwrite) {
        QFile::rename(strFile, NumerateFileName(strFile, strExt));
    }
    if (file.exists()) {
        if (file.size() == arrPic.size()) {
            LogOut("Warning - file with same name and size exist \""
                   + file.fileName() + "\". Skipping...");
            return;
        }
        LogOut("Warning - file with same name and DIFFERENT size"
               + QString("(exist '%1', new '%2') exist \"%3\". Numerate...")
               .arg(file.size()).arg(arrPic.size()).arg(file.fileName()));
        QString strNumFilePath = NumerateFileName(strFile, strExt);
        if (strNumFilePath.isEmpty()) {
            LogOut("Error - numerate function return empty name.");
            return;
        }
        file.setFileName(strNumFilePath);
    }

    if (file.open(QIODevice::WriteOnly)) {
        if (file.write(arrPic) == arrPic.size()) {
            LogOut(file.fileName() + " download success.");
            m_pFileSavedClbk->FileSaved(file.fileName());
        } else {
            LogOut("Error - failed to write file \"" + file.fileName()
                   + "\". Error string: " + file.errorString());
        }
    } else {
        LogOut("Error - failed to open file \"" + file.fileName()
               + "\". Error string: " + file.errorString());
    }
}

void FileSaver::LogOut(const QString &strMessage)
{
    if (m_pLog) {
        m_pLog->LogOut("---------[FileSaver] " + strMessage);
    }
}

QString FileSaver::GuessExtByContent(QByteArray &arrPic)
{
    QBuffer buf(&arrPic);
    auto strExt = QImageReader(&buf).format();
    if (strExt.isEmpty()) {
        LogOut("Warning - couldn't guess file extension for content");
    }
    return strExt;
}

// numerate file name unless file with this name does not exist
QString FileSaver::NumerateFileName(const QString& strFileName,
                                    const QString& strExt)
{
    for (int i = 1; i < 1000; i++) {
        auto numName(QString("%1_%2.%3").arg(strFileName).arg(i).arg(strExt));
        if (!QFile::exists(numName))
            return numName;
    }
    return "";
}
