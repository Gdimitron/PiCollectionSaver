// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once

#include "topleveiInterfaces.h"
#include "HttpDownloader.h"
#include "SerialPicsDownloader.h"
#include "plugin.h"

class Site: public ISite
{
    QString m_strDestDir;

    Plugin m_plugin;
    std::shared_ptr<ISiteInfo> m_siteInfo;
    std::shared_ptr<IUrlBuilder> m_urlBuilder;
    std::shared_ptr<IFileSysBldr> m_fileNameBuilder;

    IMainLog *m_log;
    HttpDownloader m_httpDwnld;
    SerialPicsDownloader m_serPicDwnld;

public:
    Site(const QString &type, const QString &strDestDir, IMainLog *log,
         IFileSavedCallback *fileSavedClbk );
    virtual ~Site() {}

    const ISiteInfo *SiteInfo() { return m_siteInfo.get(); }
    const IUrlBuilder *UrlBldr() { return m_urlBuilder.get(); }
    const IFileSysBldr *FileNameBldr() { return m_fileNameBuilder.get(); }
    std::shared_ptr<IHtmlPageElm> HtmlPageElmCtr(const QString &strContent);

    QSharedPointer<ISqLitePicPreview> GetSqLitePreviewCache();

    QSharedPointer<ISqLiteManager> DB();
    ISerialPicsDownloader* SerialPicsDwnld();

    bool DownloadPicLoopWithWait(const qListPairOf2Str &picPageLinkFileName);
    bool ProcessUser(QPair<QString, QString> prUsrsActvTime);
};
