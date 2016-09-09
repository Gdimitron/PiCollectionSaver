// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "picollectionsaver.h"
#include "errhlpr.h"
#include "AddNewUser.h"
#include "DownloadPicById.h"
#include "site.h"
#include "plugin.h"

#include <QDir>
#include <QTimer>
#include <QSqlTableModel>
#include <QScrollBar>

PiCollectionSaver::PiCollectionSaver(QWidget *parent)
    : QMainWindow(parent), m_iProccessing(0)
{
    ui.setupUi(this);
    ui.progressBar->setVisible(false);

    m_previewSqLiteCache = ISqLitePicPreviewCtr(this, this);
    m_previewDownload.reset(new PreviewPicTable(ui.textBrowserDownloaded));
    m_previewBrowse.reset(new PreviewPicTable(ui.textBrowserPicViewer));

    connect(ui.pushButtonStartStopAll, SIGNAL(clicked()), this,
            SLOT(slotStartStopAll()));

    connect(ui.listWidgetPicViewer, SIGNAL(itemSelectionChanged()), this,
            SLOT(slotPicViewerItemSelectionChanged()));
    connect(ui.lineEditPicViewer, SIGNAL(returnPressed()), this,
            SLOT(slotPicViewerReturnPressed()));
    connect(ui.tabWidget, SIGNAL(currentChanged(int)), this,
            SLOT(slotTabChanged(int)));
    connect(ui.textBrowserPicViewer, SIGNAL(anchorClicked(const QUrl &)), this,
            SLOT(slotTextBrowserPicAnchorClicked(const QUrl &)));
    ui.textBrowserPicViewer->installEventFilter(this);

    auto lstSiteType = Plugin::GetPluginTypes();
    if (lstSiteType.size() == 1) {
        m_strSiteType = lstSiteType.at(0);
    } else if (lstSiteType.size() > 1) {
        //TODO: add dialog for choosing site type
        m_strSiteType = lstSiteType.at(0);
        Q_ASSERT(m_strSiteType == "DeviantArt");
    }

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

const QString &PiCollectionSaver::GetWD() const
{
    static QString strPath;
    auto editVal = ui.lineEditDestinationFolder->text();
    if (strPath != editVal) {
        strPath = editVal;
        if (!strPath.isEmpty() && strPath[0] == '~') {
            strPath = strPath.replace("~", QDir::homePath());
        }
    }
    return strPath;
}

void PiCollectionSaver::FileSaved(const QString &strPath)
{
    QByteArray preview;
    QFileInfo fileInfo(strPath);
    m_previewSqLiteCache->GetBase64Preview(fileInfo.fileName(), preview);
    m_previewDownload->AddPreviewPic(strPath, preview);
}

bool PiCollectionSaver::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
        QPoint numSteps = wheelEvent->angleDelta() / (8 * 15);
        qDebug("Wheel event %d %d", numSteps.x(), numSteps.y());
        return true;
    } else {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }
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
        site.reset(new Site(m_strSiteType, GetWD(), this, this));
    } catch (const std::bad_function_call &ex) {
        CErrHlpr::Critical("Failed to load site plugin.", ex.what());
        ProcessingStopped();
        return;
    }
    site->SerialPicsDwnld()->SetOverwriteMode(false);

    auto db = site->DB();
    int iTotalUsers(db->GetUserCnt()), iProcessedUsersCnt(0);
    const int iSqlGetMaxCnt(20);

    auto lstUsrsActvTime = site->DB()->GetFirstAllUsersIdActivityTime(
                iSqlGetMaxCnt, bFavorite, bEmptyActivityTime);
    while(!lstUsrsActvTime.isEmpty()) {
        foreach(auto prUsrsActvTime, lstUsrsActvTime) {
            m_previewDownload->StartProcessUser(prUsrsActvTime.first);
            if (!site->ProcessUser(prUsrsActvTime) || IsStopProcessing()) {
                goto exit;
            }
            ui.progressBar->setValue(++iProcessedUsersCnt * 100 / iTotalUsers);
        }
        lstUsrsActvTime = db->GetNextAllUsersIdActivityTime(
                    iSqlGetMaxCnt, bFavorite, bEmptyActivityTime);
    }
exit:
    ProcessingStopped();
}

void PiCollectionSaver::on_actionAdd_new_user_s_triggered()
{
    QSharedPointer<Site> site;
    try {
        site.reset(new Site(m_strSiteType, GetWD(), this, this));
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
        site.reset(new Site(m_strSiteType, GetWD(), this, this));
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

void PiCollectionSaver::slotPicViewerItemSelectionChanged()
{
    ui.textBrowserPicViewer->clear();

    if (ui.listWidgetPicViewer->selectedItems().isEmpty())
        return;

    QString strPicPath = GetWD() +
            "/" + ui.listWidgetPicViewer->selectedItems().first()->text();

    QSize szBrowser = ui.textBrowserPicViewer->size();
    ui.textBrowserPicViewer->setHtml(QString("<img src=\"%1\" height=\"%2\" >")
                                     .arg(strPicPath).arg(szBrowser.height()));
}

void PiCollectionSaver::slotPicViewerReturnPressed()
{
    ui.listWidgetPicViewer->clear();
    QString destDir = GetWD();
    QDir dir(destDir);
    QStringList lstFiles = dir.entryList((ui.lineEditPicViewer->text()
                                          //TODO: add proper extension handling
                                          + "*.jp*g").split(" "), QDir::Files);

    QEventLoop loop;
    QByteArray preview;
    m_previewBrowse->StartProcessUser(ui.lineEditPicViewer->text());
    for(auto strFileName: lstFiles) {
        m_previewSqLiteCache->GetBase64Preview(strFileName, preview);
        m_previewBrowse->AddPreviewPic(destDir + "/" + strFileName, preview);

        QTimer::singleShot(5/*ms*/, &loop, SLOT(quit())); // to unfreeze gui
        loop.exec();
    }
}

void PiCollectionSaver::slotTabChanged(int)
{
    return;
}

void PiCollectionSaver::slotTextBrowserPicAnchorClicked(const QUrl &link)
{
    static int val;
    QString strLink = link.toString();
    if (strLink == "gallery") {
        m_previewBrowse->RepaintPreviewGallery();
        ui.textBrowserPicViewer->verticalScrollBar()->setValue(val);
        return;
    }
    val = ui.textBrowserPicViewer->verticalScrollBar()->value();
    QSize szBrowser = ui.textBrowserPicViewer->size();
    ui.textBrowserPicViewer->setHtml(
                QString("<a href=\"gallery\"><img src=\"%1\" height=\"%2\"></a>")
                .arg(strLink).arg(szBrowser.height()));
    return;
}
