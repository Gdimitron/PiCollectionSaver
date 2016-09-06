#pragma once

#include <QString>
#include <QTextBrowser>

class PreviewPicTable
{
    QTextBrowser *m_pTextBrows;
    QString m_strContentForUser;
    QString m_strUser;

public:
    PreviewPicTable(QTextBrowser *pTextBrows);

    void StartProcessUser(const QString &strUsr);
    void AddPreviewPic(const QString &picPath, const QByteArray &picBase64);
    void RepaintPreviewGallery();

private:
    void AddHeading(const QString &text);
};
