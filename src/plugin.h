// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once

#include "topleveiInterfaces.h"

// Forward declaration
class QLibrary;

class Plugin
{
    std::shared_ptr<QLibrary> m_pPlugLib;
    f_ISiteInfoCtr m_fSiteInfo;
    f_IFileSysBldrCtr m_fFsBldCtr;
    f_IUrlBuilderCtr m_fUrlBldr;
    f_IHtmlPageElmCtr m_fHtmlPageElm;

    static QString plugDir;
    static QString plugNamePattern;
    static QString plugExt;

public:
    Plugin(const QString &type, IMainLog *log);
    std::shared_ptr<ISiteInfo> GetSiteInfo() const;
    std::shared_ptr<IUrlBuilder> GetUrlBuilder() const;
    std::shared_ptr<IFileSysBldr> GetFileNameBuilder() const;
    std::shared_ptr<IHtmlPageElm> GetHtmlPageElm(
            const QString &strContent) const;

    static QStringList GetPluginTypes();
};
