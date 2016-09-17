// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include <QDialog>

#include "HttpDownloader.h"
#include "topleveiInterfaces.h"

namespace Ui {
class DirectPicPageDownload;
}

class DownloadPicById : public QDialog
{
    Q_OBJECT
    
public:
    explicit DownloadPicById(QWidget *parent, ISite *pSite, IMainLog *pLog);
    ~DownloadPicById();

    QMap<QString, QString> Impl();
    
private:
    Ui::DirectPicPageDownload *ui;

    ISite *m_pSite;
    HttpDownloader m_Http;
    IMainLog *m_pLog;
};
