#pragma once

#include <QString>
#include <QTextBrowser>

class PreviewPicTable
{
    QTextBrowser *m_pTextBrows;
    QTextCursor m_tCursor;
    QString m_strUser;

public:
    PreviewPicTable(QTextBrowser *pTextBrows);

    void Clear();
    void NewUser(const QString &strUsr);
    void AddPreviewPic(const QString &picPath, const QByteArray &picBase64);
};
