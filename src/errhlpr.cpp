// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "errhlpr.h"
#include <qmessagebox.h>

bool CErrHlpr::IgnMsgBox(const QString& strText,
                         const QString& strInformativeText)
{
    QString strInfoText = strInformativeText;
    QRegExp linkRegExp("http://[ \"]*");

    for (int pos = 0; (pos = linkRegExp.indexIn(strInfoText, pos)) != -1;) {
        QString strHref("<a href=\"" + linkRegExp.cap(1) + "\">"
                        + linkRegExp.cap(1) + "</a>");
        strInfoText.replace(linkRegExp.cap(1), strHref);
        pos += strHref.length();
    }

    QMessageBox msgBox(QMessageBox::Warning, "Warning", strText);

    msgBox.setInformativeText(strInfoText);
    msgBox.setStandardButtons(QMessageBox::Ignore | QMessageBox::Abort);

    return msgBox.exec() == QMessageBox::Ignore;
}

void CErrHlpr::Critical(const QString& strText, const QString& strDetailedText)
{
    QMessageBox msgBox(QMessageBox::Critical, "Critical Error",
                       "****** Critical error ******");

    msgBox.setInformativeText(strText);
    msgBox.setDetailedText(strDetailedText);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();

    qCritical("%s", strText.toUtf8().constData());
}

void CErrHlpr::Fatal(const QString& strText, const QString& strDetailedText)
{
    QMessageBox msgBox(QMessageBox::Critical, "Fatal Error",
                       "****** Fatal error."
                       "Unfortunately, continue is not possible. ******");

    msgBox.setInformativeText(strText);
    msgBox.setDetailedText(strDetailedText);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();

    qFatal("%s", strText.toUtf8().constData());
}
