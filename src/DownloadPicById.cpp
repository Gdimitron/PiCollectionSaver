// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "DownloadPicById.h"
#include "ui_DownloadPicById.h"

#include <QTextStream>

#include "errhlpr.h"
#include "CommonUtils.h"

DownloadPicById::DownloadPicById(QWidget *parent, ISite *pSite, IMainLog *pLog)
    : QDialog(parent), ui(new Ui::DirectPicPageDownload), m_pSite(pSite),
    m_Http(this), m_pLog(pLog)
{
    ui->setupUi(this);
    setModal(true);
}

DownloadPicById::~DownloadPicById()
{
    delete ui;
}

qListPairOf2Str DownloadPicById::Impl()
{
    qListPairOf2Str lstUrlFileName;
    if (exec() == QDialog::Accepted) {
        QTextStream strStreamPicsIds(
                    ui->plainTextEditPicPageUrls->toPlainText().toUtf8());

        for (QString strLine = strStreamPicsIds.readLine();
             !strLine.isNull();
             strLine = strStreamPicsIds.readLine())
        {
            if (strLine.isEmpty())
                continue;

            m_pLog->LogOut("(DownloadPicById::Impl) Process line: " + strLine);

            QString strPicPageUrl;

            if (strLine.indexOf("?item=") != -1) {
                // line is picture url
                strPicPageUrl = strLine;
            }
            else if (strLine.indexOf(QRegExp("\\D")) == -1) {
                // line with digits only (suppose that line is pic ID)
                strPicPageUrl = QsFrWs(m_pSite->UrlBldr()->GetPicUrlByPicId(
                                           strLine.toStdWString()));
            }

            if (strPicPageUrl.isEmpty()) {
                if (CErrHlpr::IgnMsgBox("Empty picID was returned from line...",
                                        strLine)){
                    continue;
                }
                break;
            }

            m_Http.ObtainAuthCookie(
                        QsFrWs(m_pSite->SiteInfo()->GetProtocolHostName()),
                        QsFrWs(m_pSite->SiteInfo()->GetAuthInfo()));
            QByteArray byteRep = m_Http.DownloadSync(QUrl(strPicPageUrl));
            QString strRep = CommonUtils::Win1251ToQstring(byteRep);
            if (strRep.isEmpty()) {
                if (CErrHlpr::IgnMsgBox(
                            "Could not download page",
                            "URL: " + strPicPageUrl)) {
                    continue;
                }
                break;
            }

            auto htmlElement(m_pSite->HtmlPageElmCtr(strRep));
            QString strUserId;
            try {
                strUserId = QsFrWs(htmlElement->GetUserIdPicPage());
            } catch (const parse_ex& ex) {
                if (CErrHlpr::IgnMsgBox("Could not find user ID for url",
                                        "URL: " + strPicPageUrl)) {
                    continue;
                }
                break;
            }
            Q_ASSERT(!strUserId.isEmpty());
            auto strFile(QsFrWs(m_pSite->FileNameBldr()->GetPicFileName(
                                    strUserId.toStdWString(),
                                    m_pSite->UrlBldr()->GetPicIdFromUrl(
                                        strPicPageUrl.toStdWString()))));

            lstUrlFileName.append(qMakePair(strPicPageUrl, strFile));

            m_pLog->LogOut("(DownloadPicById::Impl) Add pic to list: "
                           + strUserId + " (" + strFile + ")");
        }
    }
    return lstUrlFileName;
}
