// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include "topleveiInterfaces.h"

class AlbumManager: public IAlbmMngr
{
    ISite *m_pSite;
    IMainLog *m_pLog;
    QString m_strFolder;
    QString m_strUserId;
    std::shared_ptr<IHtmlPageElm> m_htmlElmUsrMain;
    HttpDownloader *m_pHttpDown;

public:
    AlbumManager(ISite *pSite, IMainLog *pLog,
                 const QString &strFolder, const QString &strUserId,
                 const std::shared_ptr<IHtmlPageElm> &htmlElementUserMain,
                 HttpDownloader *pHttpDown);

    QMap<QString, QString> GetMissingPicPageUrlLst();

private: // methods
    void LogOut(const QString &strMessage);
};
