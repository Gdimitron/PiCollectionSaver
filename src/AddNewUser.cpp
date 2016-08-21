// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include <QTextStream>

#include "AddNewUser.h"
#include "CommonConstants.h"
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
    if (exec() == QDialog::Accepted)
    {
        QTextStream strStreamUsers(
                    m_ui_dialog.plainTextEditNewUsers->toPlainText().toUtf8());

        for (QString strLine = strStreamUsers.readLine();
            !strLine.isNull();
            strLine = strStreamUsers.readLine())
        {
            if (strLine.isEmpty())
                continue;

            m_pLog->LogOut("(AddNewUser::Impl) Process line: " + strLine);

            QString strId;
            if (strLine.indexOf("id=") != -1) {
                // get ID from link
                strId = m_pSite->UrlBldr()->GetUserIdFromUrl(strLine);
            }
            else if (strLine.indexOf(QRegExp("\\D")) == -1) {
                // line with digits only(suppose that line is ID)
                strId = strLine;
            }

            if (strId.isEmpty()) {
                if (CErrHlpr::IgnMsgBox(
                            "Empty ID was returned from line...", strLine)) {
                    continue;
                }
                break;
            }

            if (!m_http) {
                m_http = new HttpDownloader(this);
            }

            QString strMainPageUrl =
                    m_pSite->UrlBldr()->GetMainUserPageUrlById(strId);
            QByteArray byteRep = m_http->DownloadSync(QUrl(strMainPageUrl));
            QString strRep = CommonUtils::Win1251ToQstring(byteRep);

            if (strRep.isEmpty()) {
                if (CErrHlpr::IgnMsgBox(
                            "Could not download page for ID " + strId,
                            "URL: " + strMainPageUrl)) {
                    continue;
                }
                break;
            }

            auto htmlElement(m_pSite->HtmlPageElmCtr(strRep));
            Q_ASSERT(!htmlElement->IsSelfNamePresent());

            QString strUserName = htmlElement->GetUserName();
            if (!strUserName.isEmpty())
            {
                if (m_pSite->DB()->IsUserWithIdExist(strId)) {
                    m_pLog->LogOut("(AddNewUser::Impl) " + strUserName
                                   + " with ID " + strId + " already exist");
                    QPair<QString, QString> prUsrActiveTime;
                    prUsrActiveTime.first = strId;
                    prUsrActiveTime.second = "";
                    m_pSite->DB()->UpdateLastActivityTime(prUsrActiveTime);
                    continue;
                }


                m_pLog->LogOut("(AddNewUser::Impl) Add user to DB: "
                               + strUserName + " with ID: " + strId);

                // do not save last active time while add new user
                // - save only while "process" pictures
                m_pSite->DB()->AddNewUser(strUserName, strId, "");
            }
            else
            {
                if (CErrHlpr::IgnMsgBox(
                            "Could not parse user name for ID " + strId,
                            "URL: " + strMainPageUrl)) {
                    continue;
                }
                break;
            }
        }
    }
}
