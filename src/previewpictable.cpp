#include "previewpictable.h"

PreviewPicTable::PreviewPicTable(QTextBrowser *pTextBrows)
    : m_pTextBrows(pTextBrows), m_tCursor(m_pTextBrows->textCursor())
{
}

void PreviewPicTable::Clear()
{
    m_pTextBrows->clear();
    m_tCursor = m_pTextBrows->textCursor();
    m_strUser.clear();
}

void PreviewPicTable::NewUser(const QString &strUsr)
{
    const static QString h2Tag = "<br><u><h2 align=\"center\">%1</h2></u><br>";
    QString strBlock = h2Tag.arg(strUsr);
    if (m_strUser.isEmpty()) {
        strBlock = strBlock.replace("<br><u>", "<u>");
    }
    m_tCursor.insertHtml(strBlock);
    m_strUser = strUsr;
}

void PreviewPicTable::AddPreviewPic(const QString &picPath,
                                    const QByteArray &picBase64)
{
    const static QString imgTag(
                "<a href=\"%1\"><img src=\"data:image/jpg;base64,%2\"></a> ");
    QString strBlock = imgTag.arg(picPath).arg(picBase64.constData());
    m_tCursor.insertHtml(strBlock);
}
