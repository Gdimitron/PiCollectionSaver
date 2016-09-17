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
    QSharedPointer<ISqLiteManager> m_db;

public:
    Site(const QString &type, const QString &strDestDir, IMainLog *log,
         IFileSavedCallback *fileSavedClbk );
    virtual ~Site() {}

    const ISiteInfo *SiteInfo() const { return m_siteInfo.get(); }
    const IUrlBuilder *UrlBldr() const { return m_urlBuilder.get(); }
    const IFileSysBldr *FileNameBldr() const { return m_fileNameBuilder.get(); }
    std::shared_ptr<IHtmlPageElm> HtmlPageElmCtr(
            const QString &strContent) const;

    ISqLiteManager * DB();
    ISerialPicsDownloader* SerialPicsDwnld();

    bool DownloadPicLoopWithWait(
            const QMap<QString, QString> &mapPicPageUrlFileName);
    bool ProcessUser(const QString &strUserId, const QString &strActivTime);
};
