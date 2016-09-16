#pragma once

#include <QString>
#include <QTextBrowser>

enum imgMode { modeHtmlEmbdedImg, modeDocResourcesImg };

class PreviewPicTable
{
    imgMode m_imgMode;
    QTextBrowser *m_pTextBrows;
    QTextCursor m_tCursor;
    QString m_strUser;

    bool m_bUseBlockCache;
    const int m_CacheBlockCntThresholdInitial;
    int m_CacheBlockCntThreshold;
    int m_CacheBlockCnt;
    QString m_strCachedBlocks;

public:
    PreviewPicTable(QTextBrowser *pTextBrows);

    void Clear();
    void NewUser(const QString &strUsr);
    void AddPreviewPic(const QString &imgPath, const QByteArray &imgPreview);
    void CachedModeEnable(bool bEnable);

private:
    void AddHeader();
    void InsertCachedBlocks();
};
