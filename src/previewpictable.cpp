#include "previewpictable.h"

const QString c_imgHeight("200");

PreviewPicTable::PreviewPicTable(QTextBrowser *pTextBrows)
    : m_pTextBrows(pTextBrows)
{
    //TODO: try to figure out why this is not working
    m_pTextBrows->document()->setDefaultStyleSheet(
                "<style type='text/css'>img { padding:1px; "
                "border:1px solid #021a40; }</style>");
}

void PreviewPicTable::StartProcessUser(const QString &strUsr)
{
    if (m_strUser != strUsr) {
        m_pTextBrows->clear();
        m_strContentForUser.clear();
        m_strUser = strUsr;
    }
    AddHeading(strUsr);
}

void PreviewPicTable::AddHeading(const QString &text)
{
    static QString h2Tag = "<h2><u>%1</u></h2><br>";
    QString strBlock = h2Tag.arg(text);
    m_strContentForUser += strBlock;
    m_pTextBrows->textCursor().insertHtml(strBlock);
}

void PreviewPicTable::AddPreviewPic(const QString &picPath,
                                    const QByteArray &picBase64)
{
    static QString imgTag(
                "<a href=\"%1\"><img align=\"middle\" height=\"" + c_imgHeight
                + "\" src=\"data:image/jpg;base64,%2\"> </a>");
    QString strBlock = imgTag.arg(picPath).arg(picBase64.constData());
    m_strContentForUser += strBlock;
    m_pTextBrows->textCursor().insertHtml(strBlock);
}

void PreviewPicTable::RepaintPreviewGallery()
{
    m_pTextBrows->clear();
    m_pTextBrows->textCursor().insertHtml(m_strContentForUser);
}
