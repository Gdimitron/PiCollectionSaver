// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include <QTextStream>

#include "AddNewUser.h"
#include "CommonUtils.h"
#include "errhlpr.h"

AddNewUser::AddNewUser(QWidget *parent, ISite *pSite, IMainLog *pLog)
    : QDialog(parent), m_pSite(pSite), m_http(NULL), m_pLog(pLog)
{
    m_ui_dialog.setupUi(this);
    setModal(true);
}

void AddNewUser::Impl()
{
    if (exec() != QDialog::Accepted) {
        return;
    }
    QTextStream strStreamUsers(
                m_ui_dialog.plainTextEditNewUsers->toPlainText().toUtf8());

    auto db = m_pSite->DB();
    for (QString strLine = strStreamUsers.readLine();
        !strLine.isNull();
        strLine = strStreamUsers.readLine())
    {
        if (strLine.isEmpty())
            continue;

        m_pLog->LogOut("(AddNewUser::Impl) Process line: " + strLine);

        QString strId;
        if (strLine.indexOf(QRegExp("\\D")) == -1) {
            // line with digits only(suppose that line is ID)
            strId = strLine;
        } else {
            strId = QsFrWs(m_pSite->UrlBldr()->GetUserIdFromUrl(
                               strLine.toStdWString()));
        }

        if (strId.isEmpty()) {
            if (CErrHlpr::IgnMsgBox("Empty name or ID was returned for...",
                                    strLine)) {
                continue;
            }
            break;
        }

        QString strUserName(strId);
        if ( !m_pSite->SiteInfo()->UsersIdAndNameSame() ) {
            QString strMainPageUrl = QsFrWs(
                        m_pSite->UrlBldr()->GetMainUserPageUrlById(
                            strId.toStdWString()));
            strUserName = ObtainUserName(strMainPageUrl);
            if (strUserName.isEmpty()) {
                if (CErrHlpr::IgnMsgBox("Could not get user name for ID "
                                        + strId, "URL: " + strMainPageUrl)) {
                    continue;
                }
                break;
            }
        }

        if (db->IsUserWithIdExist(strId)) {
            m_pLog->LogOut("(AddNewUser::Impl) " + strUserName
                           + " with ID " + strId + " already exist");
            QPair<QString, QString> prUsrActiveTime;
            prUsrActiveTime.first = strId;
            prUsrActiveTime.second = "";
            db->UpdateLastActivityTime(prUsrActiveTime);
            continue;
        }

        m_pLog->LogOut("(AddNewUser::Impl) Add user to DB: "
                       + strUserName + " with ID: " + strId);

        // do not save last active time while add new user
        // - save only while "process" pictures
        db->AddNewUser(strUserName, strId, "");
    }
}

QString AddNewUser::ObtainUserName(const QString &strMainPageUrl)
{
    if (!m_http) {
        m_http = new HttpDownloader(this);
    }
    QByteArray byteRep = m_http->DownloadSync(QUrl(strMainPageUrl));
    QString strRep = CommonUtils::Win1251ToQstring(byteRep);
    if (strRep.isEmpty()) {
        return "";
    }
    auto htmlElement(m_pSite->HtmlPageElmCtr(strRep));
    Q_ASSERT(!htmlElement->IsSelfNamePresent());
    return QsFrWs(htmlElement->GetUserName());
}
