// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include <QDialog>
#include "ui_AddNewUserDlg.h"

#include "HttpDownloader.h"
#include "topleveiInterfaces.h"

class AddNewUser : public QDialog
{
    Q_OBJECT

private:
    Ui::DialogAddNewUser m_ui_dialog;
    ISite *m_pSite;
    HttpDownloader *m_http;
    IMainLog *m_pLog;

public:
    explicit AddNewUser(QWidget *parent = 0, ISite *pSite = 0,
                        IMainLog *pLog = 0);
    void Impl();

private:
    QString ObtainUserName(const QString &strMainPageUrl);

signals:
    
public slots:
    
};
