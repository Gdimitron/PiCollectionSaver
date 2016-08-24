// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include <QLibrary>

#include "topleveiInterfaces.h"
#include "HttpDownloader.h"
#include "SerialPicsDownloader.h"

class Site: public ISite
{
    QString m_strDestDir;
    std::shared_ptr<IUrlBuilder> m_urlBuilder;
    std::shared_ptr<IFileSysBldr> m_fileNameBuilder;
    std::shared_ptr<ISiteInfo> m_siteInfo;

    QLibrary m_PlugLib;
    f_IHtmlPageElmCtr m_fHtmlPageElm;

    IMainLog *m_log;
    QSharedPointer<ISqLiteManager> m_DB;
    HttpDownloader m_httpDwnld;
    SerialPicsDownloader m_serPicDwnld;

public:
    Site(const QString &type, const QString &strDestDir, IMainLog *log,
         IFileSavedCallback *fileSavedClbk );
    virtual ~Site() {}

    std::shared_ptr<ISiteInfo> SiteInfo();
    std::shared_ptr<IUrlBuilder> UrlBldr();
    std::shared_ptr<IFileSysBldr> FileNameBldr();
    std::shared_ptr<IHtmlPageElm> HtmlPageElmCtr(const QString &strContent);

    QSharedPointer<ISqLiteManager> DB();
    ISerialPicsDownloader* SerialPicsDwnld();

    bool DownloadPicLoopWithWait(const qListPairOf2Str &picPageLinkFileName);
    bool ProcessUser(QPair<QString, QString> prUsrsActvTime);
};
