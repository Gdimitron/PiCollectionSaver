// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "picollectionsaver.h"
#include "errhlpr.h"
#include "AddNewUser.h"
#include "DownloadPicById.h"
#include "CommonConstants.h"
#include "site.h"

#include <QDir>
#include <QSqlTableModel>

PiCollectionSaver::PiCollectionSaver(QWidget *parent)
    : QMainWindow(parent), m_strSiteType("PRV"), m_iProccessing(0)
{
    ui.setupUi(this);
    ui.progressBar->setVisible(false);

    connect(ui.pushButtonStartStopAll, SIGNAL(clicked()), this,
            SLOT(slotStartStopAll()));

    connect(ui.listWidgetDownloaded, SIGNAL(itemSelectionChanged()), this,
            SLOT(slotDownloadedItemSelectionChanged()));

    connect(ui.listWidgetPicViewer, SIGNAL(itemSelectionChanged()), this,
            SLOT(slotPicViewerItemSelectionChanged()));
    connect(ui.lineEditPicViewer, SIGNAL(returnPressed()), this,
            SLOT(slotPicViewerReturnPressed()));
    connect(ui.tabWidget, SIGNAL(currentChanged(int)), this,
            SLOT(slotTabChanged(int)));

    //    CSqLiteManager db(szDbFileName, szDbTableName);
    //    QSqlTableModel *model = new QSqlTableModel(this, db.getDb());
    //    model->setTable(szDbTableName);
    //    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    //    model->select();

    //    ui.tableView->setModel(model);
    //    ui.tableView->hideColumn(0); // don't show the ID
    //    ui.tableView->show();
}

PiCollectionSaver::~PiCollectionSaver()
{
}

void PiCollectionSaver::LogOut(const QString &strMessage)
{
    QDebug(QtDebugMsg) << strMessage;

    if (strMessage.contains("warning", Qt::CaseInsensitive)) {
        ui.plainTextEditLog->appendHtml("<b>" + strMessage + "</b>");
    } else if (strMessage.contains("error", Qt::CaseInsensitive)) {
        ui.plainTextEditLog->appendHtml("<b><font color=\"red\">" +
                                        strMessage + "</font></b>");
    } else {
        ui.plainTextEditLog->appendHtml(strMessage);
    }
}

void PiCollectionSaver::FileSaved(const QString &strPath)
{
    QListWidgetItem *qtItem = new QListWidgetItem(ui.listWidgetDownloaded);
    qtItem->setText(strPath);
}

void PiCollectionSaver::slotStartStopAll(void)
{
    if (IsProcessing())
        StopProcessing();
    else
        ProcessAllUsers(false, false);
}

void PiCollectionSaver::StartProcessing()
{
    ui.pushButtonStartStopAll->setText("Stop");
    ui.progressBar->setVisible(true);
    LogOut("Processing start...");
    m_iProccessing.store(1);
}

void PiCollectionSaver::StopProcessing()
{
    LogOut("Stop processing request...");
    m_iProccessing.store(2);
}

void PiCollectionSaver::ProcessingStopped()
{
    LogOut("Processing stopped.");
    ui.pushButtonStartStopAll->setText("Start All");
    ui.progressBar->setVisible(false);
    m_iProccessing.store(0);
}

bool PiCollectionSaver::IsStopProcessing()
{
    return m_iProccessing.load() == 2;
}

bool PiCollectionSaver::IsProcessing()
{
    return m_iProccessing.load() > 0;
}

void PiCollectionSaver::ProcessAllUsers(bool bFavorite, bool bEmptyActivityTime)
{
    StartProcessing();

    QSharedPointer<Site> site;
    try {
        site.reset(new Site(m_strSiteType, ui.lineEditDestinationFolder->text(),
                            this, this));
    } catch (const std::bad_function_call &ex) {
        CErrHlpr::Critical("Failed to load site plugin.", ex.what());
        ProcessingStopped();
        return;
    }

    site->SerialPicsDwnld()->SetOverwriteMode(false);

    int iTotalUsers(site->DB()->GetUserCnt()), iProcessedUsersCnt(0);
    const int iSqlGetMaxCnt(20);

    auto lstUsrsActvTime = site->DB()->GetFirstAllUsersIdActivityTime(
                iSqlGetMaxCnt, bFavorite, bEmptyActivityTime);
    while(!lstUsrsActvTime.isEmpty()) {
        foreach(auto prUsrsActvTime, lstUsrsActvTime) {
            if (!site->ProcessUser(prUsrsActvTime) || IsStopProcessing()) {
                goto exit;
            }
            ui.progressBar->setValue(++iProcessedUsersCnt * 100 / iTotalUsers);
        }
        lstUsrsActvTime = site->DB()->GetNextAllUsersIdActivityTime(
                    iSqlGetMaxCnt, bFavorite, bEmptyActivityTime);
    }
exit:
    ProcessingStopped();
}

void PiCollectionSaver::on_actionAdd_new_user_s_triggered()
{
    QSharedPointer<Site> site;
    try {
        site.reset(new Site(m_strSiteType, ui.lineEditDestinationFolder->text(),
                            this, this));
    } catch (const std::bad_function_call &ex) {
        CErrHlpr::Critical("Failed to load site plugin.", ex.what());
        return;
    }
    AddNewUser *addNewUsersDlg = new AddNewUser(this, &*site, this);
    addNewUsersDlg->Impl();

    ProcessAllUsers(false, true);
}

void PiCollectionSaver::on_actionDownload_photo_by_ID_triggered()
{
    QSharedPointer<Site> site;
    try {
        site.reset(new Site(m_strSiteType, ui.lineEditDestinationFolder->text(),
                            this, this));
    } catch (const std::bad_function_call &ex) {
        CErrHlpr::Critical("Failed to load site plugin.", ex.what());
        return;
    }
    DownloadPicById *downPicByIdDlg = new DownloadPicById(this, &*site, this);
    auto qlstPrPicPageLinkFileName = downPicByIdDlg->Impl();

    if (!qlstPrPicPageLinkFileName.isEmpty()) {
        site->SerialPicsDwnld()->SetOverwriteMode(true);
        site->DownloadPicLoopWithWait(qlstPrPicPageLinkFileName);
    }
}

void PiCollectionSaver::slotDownloadedItemSelectionChanged()
{
    ui.textBrowserDownloaded->clear();

    QString strPicPath =
            ui.listWidgetDownloaded->selectedItems().first()->text();
    QSize szBrowser = ui.textBrowserDownloaded->size();
    ui.textBrowserDownloaded->setHtml(QString("<img src=\"%1\" height=\"%2\" >")
                                      .arg(strPicPath).arg(szBrowser.height()));
}

void PiCollectionSaver::slotPicViewerItemSelectionChanged()
{
    ui.textBrowserPicViewer->clear();

    if (ui.listWidgetPicViewer->selectedItems().isEmpty())
        return;

    QString strPicPath = ui.lineEditDestinationFolder->text() +
            "/" + ui.listWidgetPicViewer->selectedItems().first()->text();

    QSize szBrowser = ui.textBrowserPicViewer->size();
    ui.textBrowserPicViewer->setHtml(QString("<img src=\"%1\" height=\"%2\" >")
                                     .arg(strPicPath).arg(szBrowser.height()));
}

void PiCollectionSaver::slotPicViewerReturnPressed()
{
    ui.listWidgetPicViewer->clear();
    QDir dir(ui.lineEditDestinationFolder->text());
    QStringList lstFiles = dir.entryList((ui.lineEditPicViewer->text()
                                          + "*.jp*g").split(" "), QDir::Files);

    foreach(QString strFileName, lstFiles) {
        QListWidgetItem *qtItem = new QListWidgetItem(ui.listWidgetPicViewer);
        qtItem->setText(strFileName);
    }
}

void PiCollectionSaver::slotTabChanged(int iIndex)
{
    if (iIndex == ui.tabWidget->indexOf(ui.tab_3)) {
        if (ui.listWidgetDownloaded->selectedItems().isEmpty())
            return;

        QString strSl(ui.listWidgetDownloaded->selectedItems().first()->text());
        int iIndex = strSl.indexOf("_");
        if (iIndex > 0) {
            QString strUserId(strSl.left(iIndex));
            iIndex = strUserId.lastIndexOf("/");
            if (iIndex != 0) {
                strUserId = strUserId.right(strUserId.size() - iIndex - 1);
            }
            ui.lineEditPicViewer->setText(strUserId);
        }
    }
}
