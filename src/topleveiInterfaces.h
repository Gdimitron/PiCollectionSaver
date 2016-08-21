// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include <QList>
#include <QString>
#include <QStringList>
#include <QPair>
#include <QSharedPointer>

#include "plugin_interface.h"
#include "HttpDownloader.h"

// typedefs:
typedef QList< QPair<QString, QString> > qListPairOf2Str;


// interface IMainLog
struct IMainLog
{
    virtual void LogOut(const QString &strMessage) = 0;
};


// interface IFileSavedCallback
struct IFileSavedCallback
{
    virtual void FileSaved(const QString &strPath) = 0;
};


// interfaces:
// interface ISqLiteManager
struct ISqLiteManager
{
    virtual bool IsUserWithIdExist(const QString& strUserId) = 0;

    virtual int GetUserCnt() = 0;

    virtual void AddNewUser(const QString & strUserName,
                            // format: "user66"
                            const QString & strUserId,
                            // format: "117235"
                            const QString & strLastActivityTime
                            // e.g: "11/07/2012 10:46:01"
                            ) = 0;

    virtual qListPairOf2Str GetFirstAllUsersIdActivityTime(
            int iCountMax,
            bool bFavoriteOnly = false,
            bool bEmptyActivityTimeOnly = false) = 0;

    virtual qListPairOf2Str GetNextAllUsersIdActivityTime(
            int iCountMax,
            bool bFavoriteOnly = false,
            bool bEmptyActivityTimeOnly = false) = 0;

    virtual void UpdateLastActivityTime(
            QPair<QString, QString> pairUserNameLastActivityTime) = 0;

    //virtual void UpdateLastActivityTimeProcessed(qListPairOf2Str) = 0;
    virtual ~ISqLiteManager() {}
};
extern QSharedPointer<ISqLiteManager> ISqLiteManagerCtr(
        const QString &strDBFileName, const QString &strTableName);

struct ISite;
// interface ISerialPicsDownloader
struct ISerialPicsDownloader
{
    virtual void SetSite(ISite *pSite) = 0;
    virtual void SetOverwriteMode(bool) = 0;
    virtual void SetDestinationFolder(const QString& strFolderPath) = 0;
    virtual void AddPicPageUrlLstToQueue(/*direct pic url & file name to save*/
            qListPairOf2Str &lstUrlPicLinksWithFileNames) = 0;
};
extern QSharedPointer<ISerialPicsDownloader> ISerPicsDownloaderCtr(
        QObject *parent, IMainLog *pLog, const ISiteInfo *pSiteInfo,
        IFileSavedCallback *pFileSaved);


// interface ISite
struct ISite
{
    virtual QSharedPointer<ISiteInfo> SiteInfo() = 0;
    virtual QSharedPointer<IUrlBuilder> UrlBldr() = 0;
    virtual QSharedPointer<IFileSysBldr> FileNameBldr() = 0;
    virtual QSharedPointer<IHtmlPageElm> HtmlPageElmCtr(const QString &) = 0;

    virtual QSharedPointer<ISqLiteManager> DB() = 0;
    virtual ISerialPicsDownloader* SerialPicsDwnld() = 0;

    virtual bool DownloadPicLoopWithWait(
            const qListPairOf2Str &picPageLinkFileName) = 0;

    virtual bool ProcessUser(QPair<QString, QString> prUsrsActvTime) = 0;
};


// interface IAlbmMngr
struct IAlbmMngr
{
    virtual qListPairOf2Str GetMissingPicPageUrlLst() = 0;
    virtual ~IAlbmMngr() {}

};
extern QSharedPointer<IAlbmMngr> IAlbmMngrCtr(
        ISite *pSite, IMainLog *pLog, const QString &strFolder,
        const QString &strUserId,
        const QSharedPointer<IHtmlPageElm> &htmlElmUserMain,
        HttpDownloader &pHttpDown);


// interface IHtmlMainPagesManager
struct IHtmlMainPageManager
{
    virtual void AddMainUserPage(QString strMainUserPageHtmlContent,
                                 QString strMainUserPageUrl,
                                 // example format: http://site.info/?user66
                                 QString strLastActivityTimeFromSql
                                 // e.g: 11/07/2012 10:46:01
                                 ) = 0;

    virtual void EraseAll(void) = 0;
    virtual qListPairOf2Str GetUserMainPageUrlsWithHtmlLastActivityTime(
            void) = 0;
    virtual QStringList GetFirstAlbumPageUrls(void) = 0;
    virtual QStringList GetAllPicsUrls(void) = 0;

private:
    virtual bool IsPageAndSqlActivityTimeSame(
            QString strMainUserPageHtmlContent,
            QString strLastActivityTimeFromSql) = 0;

    // QList<QString> main user html list // filled by AddMainUserPageHtml() method(parameter)
    // QList<QString> "all album" user html list //
    //
};
