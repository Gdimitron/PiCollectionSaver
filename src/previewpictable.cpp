#include "previewpictable.h"

#include <QFileInfo>
#include <QImage>

PreviewPicTable::PreviewPicTable(QTextBrowser *pTextBrows)
    : m_imgMode(modeDocResourcesImg), m_pTextBrows(pTextBrows),
      m_tCursor(m_pTextBrows->textCursor()), m_bUseBlockCache(false),
      m_CacheBlockCntThresholdInitial(5),
      m_CacheBlockCntThreshold(m_CacheBlockCntThresholdInitial),
      m_CacheBlockCnt(0)
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
    m_strUser = strUsr;
}

void PreviewPicTable::AddPreviewPic(const QString &imgPath,
                                    const QByteArray &imgPreview)
{
    if (!m_strUser.isEmpty()) {
        AddHeader();
    }
    QString strBlock;
    const static QString imgTag("<a href=\"%1\"><img src=\"%2\"></a> ");
    switch (m_imgMode) {
    case modeHtmlEmbdedImg:
        strBlock = imgTag.arg(imgPath).arg("data:image/jpg;base64,%2");
        strBlock = strBlock.arg(imgPreview.toBase64().constData());
        break;
    case modeDocResourcesImg:
        QImage img;
        auto resUrl = QString("thumbs://%1").arg(QFileInfo(imgPath).fileName());
        if (img.loadFromData(imgPreview)) {
            auto doc = m_pTextBrows->document();
            doc->addResource(QTextDocument::ImageResource, QUrl(resUrl),
                             QVariant(img));
        }
        strBlock = imgTag.arg(imgPath).arg(resUrl);
        break;
    }
    if (m_bUseBlockCache) {
        m_strCachedBlocks += strBlock;
        if (m_CacheBlockCnt++ > m_CacheBlockCntThreshold ) {
            m_CacheBlockCntThreshold++;
            InsertCachedBlocks();
        }
    } else {
        m_tCursor.insertHtml(strBlock);
    }
}

void PreviewPicTable::CachedModeEnable(bool bEnable)
{
    if (m_bUseBlockCache) {
        m_CacheBlockCntThreshold = m_CacheBlockCntThresholdInitial;
        // insert all cached blocks:
        InsertCachedBlocks();
    }
    m_bUseBlockCache = bEnable;
}

void PreviewPicTable::AddHeader()
{
    const static QString h2 = "<br><u><h2 align=\"center\">%1</h2></u><br>";
    QString strBlock = h2.arg(m_strUser);
    if (m_tCursor.atStart()) {
        strBlock = strBlock.replace("<br><u>", "<u>");
    }
    m_tCursor.insertHtml(strBlock);
    m_strUser.clear();
}

void PreviewPicTable::InsertCachedBlocks()
{
    m_tCursor.insertHtml(m_strCachedBlocks);
    m_strCachedBlocks.clear();
    m_CacheBlockCnt = 0;
}
