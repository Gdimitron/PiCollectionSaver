// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once

#include <QLibrary>

#include "topleveiInterfaces.h"

class Plugin
{
    QLibrary m_PlugLib;
    f_ISiteInfoCtr m_fSiteInfo;
    f_IFileSysBldrCtr m_fFsBldCtr;
    f_IUrlBuilderCtr m_fUrlBldr;
    f_IHtmlPageElmCtr m_fHtmlPageElm;

public:
    Plugin(const QString &type, IMainLog *log);
    std::shared_ptr<ISiteInfo> GetSiteInfo();
    std::shared_ptr<IUrlBuilder> GetUrlBuilder();
    std::shared_ptr<IFileSysBldr> GetFileNameBuilder();
    std::shared_ptr<IHtmlPageElm> GetHtmlPageElm(const QString &strContent);

    static QString plugDir;
    static QString plugNamePattern;
    static QString plugExt;
    static QStringList GetPluginTypes();
};
