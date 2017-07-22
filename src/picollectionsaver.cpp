// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "picollectionsaver.h"
#include "CommonConstants.h"
#include "errhlpr.h"
#include "AddNewUser.h"
#include "DownloadPicById.h"
#include "site.h"
#include "plugin.h"

#include <QDir>
#include <QTimer>
#include <QSqlTableModel>
#include <QScrollBar>
#include <QElapsedTimer>

PiCollectionSaver::PiCollectionSaver(QWidget *parent)
    : QMainWindow(parent), m_iProccessing(0)
{
    ui.setupUi(this);
    ui.progressBar->setVisible(false);

    m_previewDB = ISqLitePicPreviewCtr(this, this);
    m_previewDownload.reset(new PreviewPicTable(ui.textBrowserDownloaded));
    m_previewBrowse.reset(new PreviewPicTable(ui.textBrowserGal));

    connect(ui.pushButtonStartStopAll, SIGNAL(clicked()), this,
            SLOT(slotStartStopAll()));

    connect(ui.listWidgetPicViewer, SIGNAL(itemSelectionChanged()), this,
            SLOT(slotPicViewerItemSelectionChanged()));
    connect(ui.lineEditPicViewer, SIGNAL(returnPressed()), this,
            SLOT(slotPicViewerReturnPressed()));
    connect(ui.tabWidget, SIGNAL(currentChanged(int)), this,
            SLOT(slotTabChanged(int)));
    connect(ui.textBrowserDownloaded, SIGNAL(anchorClicked(const QUrl &)), this,
            SLOT(slotTextBrowserDownloadedAnchorClicked(const QUrl &)));
    connect(ui.textBrowserGal, SIGNAL(anchorClicked(const QUrl &)), this,
            SLOT(slotTextBrowserGalAnchorClicked(const QUrl &)));

    connect(ui.textBrowserPicViewer, SIGNAL(anchorClicked(const QUrl &)), this,
            SLOT(slotTextBrowserPicAnchorClicked(const QUrl &)));
    ui.textBrowserPicViewer->installEventFilter(this);
    ui.textBrowserPicViewer->setVisible(false);
    initPreviewBrowseItems(QStringList());

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

void PiCollectionSaver::LogOut(const QString &strMessage, bool bOnOneLine)
{
    QDebug(QtDebugMsg) << strMessage;

    QString strFormatMsg;
    if (strMessage.contains("warning", Qt::CaseInsensitive)) {
        strFormatMsg = "<b>" + strMessage + "</b>";
    } else if (strMessage.contains("error", Qt::CaseInsensitive)) {
        strFormatMsg = "<b><font color=\"red\">" + strMessage + "</font></b>";
    } else {
        strFormatMsg = strMessage;
    }
    static QString s_strOneLineMsg;
    if (bOnOneLine) {
        s_strOneLineMsg += strFormatMsg;
        return;
    }
    ui.plainTextEditLog->appendHtml(s_strOneLineMsg + strFormatMsg);
    s_strOneLineMsg.clear();
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
    if (strPath.lastIndexOf('/') != strPath.size() - 1) {
        strPath += '/';
    }
    return strPath;
}

void PiCollectionSaver::FileSaved(const QString &strPath)
{
    QByteArray preview;
    QFileInfo fileInfo(strPath);
    m_previewDB->GetPreview(fileInfo.fileName(), preview);
    m_previewDownload->AddPreviewPic(strPath, preview);
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

    int iTotalUsers(site->DB()->GetUserCnt()), iProcessedUsersCnt(0);
    const int iSqlGetMaxCnt(20);

    auto mapUsrs = site->DB()->GetFirstAllUsersIdActivityTime(
                iSqlGetMaxCnt, bFavorite, bEmptyActivityTime);
    while(!mapUsrs.isEmpty()) {
        for (auto i = mapUsrs.constBegin(); i != mapUsrs.constEnd(); ++i) {
            m_previewDownload->NewUser(i.key());
            if (!site->ProcessUser(i.key(), i.value()) || IsStopProcessing()) {
                goto exit;
            }
            ui.progressBar->setValue(++iProcessedUsersCnt * 100 / iTotalUsers);
        }
        mapUsrs = site->DB()->GetNextAllUsersIdActivityTime(
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
    auto mapPicPageUrlFileName = downPicByIdDlg->Impl();

    if (!mapPicPageUrlFileName.isEmpty()) {
        site->SerialPicsDwnld()->SetOverwriteMode(true);
        site->DownloadPicLoopWithWait(mapPicPageUrlFileName);
    }
}

void PiCollectionSaver::slotPicViewerItemSelectionChanged()
{
    QString strSel = ui.listWidgetPicViewer->selectedItems().first()->text();
}

void PiCollectionSaver::slotPicViewerReturnPressed()
{
    setGalVisible();
    QString destDir = GetWD();
    QDir dir(destDir);
    QStringList lstFilter;
    for (auto ext: g_imgExtensions) {
        lstFilter << ui.lineEditPicViewer->text() + "*" + QsFrWs(ext);
    }
    initPreviewBrowseItems(dir.entryList(lstFilter, QDir::Files,
                                         QDir::Time | QDir::Reversed));

    QElapsedTimer timer;
    timer.start();
    ui.lineEditPicViewer->setEnabled(false);
    auto strEditValue = ui.lineEditPicViewer->text();
    m_previewBrowse->Clear();
    m_previewBrowse->CachedModeEnable(true);
    m_previewBrowse->NewUser(ui.lineEditPicViewer->text());
    QByteArray preview;
    int processed(0);
    for(auto itFileName = m_previewBrowseItems.constBegin();
        itFileName != m_previewBrowseItems.constEnd(); itFileName++) {
        if (++processed % 50 == 0) {
            double speedImgSec = processed/(double(timer.elapsed()) / 1000);
            auto strSpeed = QString("%1/%2 (%3 img/sec)").arg(processed)
                    .arg(m_previewBrowseItems.size()).arg(speedImgSec);
            qDebug() << strSpeed;
            ui.lineEditPicViewer->setText(strSpeed);
        }
        if (!m_previewDB->GetPreview(*itFileName, preview)) {
            // there is no such pic in preview DB, add request for absent range
            m_previewDB->AddNotExistRange(m_previewBrowseItems, itFileName);
            auto bRes = m_previewDB->GetPreview(*itFileName, preview, true);
            Q_ASSERT(bRes);
        }
        qApp->processEvents();
        m_previewBrowse->AddPreviewPic(destDir + *itFileName, preview);
    }
    qDebug() << timer.elapsed();
    m_previewBrowse->CachedModeEnable(false);
    ui.lineEditPicViewer->setText(strEditValue);
    ui.lineEditPicViewer->setEnabled(true);
}

void PiCollectionSaver::slotTabChanged(int)
{
    return;
}

void PiCollectionSaver::slotTextBrowserDownloadedAnchorClicked(const QUrl &link)
{
    auto strLink = link.toString();
    QFileInfo fileInfo(strLink);
    auto strUserName = fileInfo.fileName().replace(QRegExp("([^_]+).*"), "\\1");
    ui.lineEditPicViewer->setText(strUserName);
    ui.tabWidget->setCurrentIndex(1);
    slotPicViewerReturnPressed();
    slotTextBrowserGalAnchorClicked(link);
}

void PiCollectionSaver::slotTextBrowserGalAnchorClicked(const QUrl &link)
{
    auto strLink = link.toString();
    m_pPrviewBrowseIter->toFront();
    m_pPrviewBrowseIter->findNext(QFileInfo(strLink).fileName());
    picViewerSetPic(strLink);
}

void PiCollectionSaver::slotTextBrowserPicAnchorClicked(const QUrl &link)
{
    if (!(QApplication::keyboardModifiers() & Qt::ControlModifier)) {
        setGalVisible();
        return;
    }
    picViewerSetPic(link.toString());
}

void PiCollectionSaver::setGalVisible()
{
    if (!ui.textBrowserGal->isVisible()) {
        ui.textBrowserPicViewer->setVisible(false);
        ui.textBrowserPicViewer->clear();
        ui.textBrowserGal->setVisible(true);
    }
}

void PiCollectionSaver::setPicViewerVisible()
{
    if (!ui.textBrowserPicViewer->isVisible()) {
        ui.textBrowserGal->setVisible(false);
        ui.textBrowserPicViewer->setVisible(true);
    }
}

void PiCollectionSaver::initPreviewBrowseItems(const QStringList &lstItems)
{
    m_previewBrowseItems = lstItems;
    m_pPrviewBrowseIter.reset(new QStringListIterator(m_previewBrowseItems));
}

void PiCollectionSaver::picViewerSetPic(const QString &strUrl)
{
    setPicViewerVisible();
    // show textBrowser to get correct size values
    qApp->processEvents();
    auto curHtml = ui.textBrowserPicViewer->toHtml();
    if (curHtml.indexOf(strUrl) != -1 && curHtml.indexOf("height=") != -1) {
        // same image clicked - update without scaling
        ui.textBrowserPicViewer->setHtml(
                    QString("<a href=\"%1\"><img src=\"%1\"></a><br>")
                    .arg(strUrl));
    } else {
        auto szBrowser = ui.textBrowserPicViewer->size();
        szBrowser.rheight() -= 30; // to prevent scroll bars
        szBrowser.rwidth() -= 15;
        QImage img(strUrl);
        auto szImage = img.size();
        auto szExpanded = szBrowser.expandedTo(szImage);
        if (szExpanded == szBrowser) {
            // no need to scale
            ui.textBrowserPicViewer->setHtml(
                        QString("<a href=\"%1\"><img src=\"%1\"></a><br>")
                        .arg(strUrl));
            return;
        }
        szImage.scale(szBrowser, Qt::KeepAspectRatio);
        ui.textBrowserPicViewer->setHtml(
                    QString("<a href=\"%1\"><img src=\"%1\" height=\"%2\""
                            " width=\"%3\"></a> <br>")
                    .arg(strUrl).arg(szImage.height()).arg(szImage.width()));
    }
}

void PiCollectionSaver::picViewerNext(bool bValInverted)
{
    if (bValInverted && m_pPrviewBrowseIter->hasNext()) {
         // direction changed, adjust java-style iterator
        m_pPrviewBrowseIter->next();
    }
    if (m_pPrviewBrowseIter->hasNext()) {
        auto strUrl = GetWD() + m_pPrviewBrowseIter->next();
        picViewerSetPic(strUrl);
    }
}

void PiCollectionSaver::picViewerPrevios(bool bValInverted)
{
    if (bValInverted && m_pPrviewBrowseIter->hasPrevious()) {
        // direction changed, adjust java-style iterator
        m_pPrviewBrowseIter->previous();
    }
    if (m_pPrviewBrowseIter->hasPrevious()) {
        auto strUrl = GetWD() + m_pPrviewBrowseIter->previous();
        picViewerSetPic(strUrl);
    }
}

bool PiCollectionSaver::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel) {
        //TODO: find out why method receive two events
        static int filter(0);
        if (!ui.textBrowserPicViewer->isVisible() || filter++ % 2) {
            return true;
        }
        QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
        QPoint numSteps = wheelEvent->angleDelta() / (8 * 15);
        static int prevVal = numSteps.y();
        if (numSteps.y() < 0) {
            picViewerNext(prevVal * numSteps.y() < 0);
        } else {
            picViewerPrevios(prevVal * numSteps.y() < 0);
        }
        prevVal = numSteps.y();
        return true;
    } else {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }
}
