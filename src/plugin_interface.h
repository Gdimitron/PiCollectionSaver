// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include <functional>
#include <exception>
#include <QSharedPointer>
#include <QString>

#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))

class parse_ex: public std::exception
{
    QString m_strWhat, m_strDetails;
public:
    parse_ex(const QString &strWhat, const QString &strDetails)
        : m_strWhat(strWhat), m_strDetails(strDetails) { }
    const char* what() const noexcept { return m_strWhat.toUtf8(); }
    const QString *strWhat()const noexcept { return &m_strWhat; }
    const QString *strDetails()const noexcept { return &m_strDetails; }
};


// interface ISiteInfo
// return site related constants (host, login, etc)
struct ISiteInfo
{
    virtual const QString &GetHostName() const = 0;

    virtual const QString &GetLogin() const = 0;
    virtual const QString &GetAuthInfo() const = 0;

    virtual const QString &GetDBFileName() const = 0;
    virtual const QString &GetDBTableName() const = 0;

    virtual ~ISiteInfo() {}
};
extern "C" QSharedPointer<ISiteInfo> ISiteInfoCtr();
typedef QSharedPointer<ISiteInfo> (ISiteInfoCtr_t)();
typedef std::function<ISiteInfoCtr_t> f_ISiteInfoCtr;


// interface IFileSysBldr some function to build file name
struct IFileSysBldr
{
    virtual QString GetPicFileName(const QString& strUserId,
                                   const QString& strFileId) = 0;

    virtual QString GetUserId(const QString& strFileName) = 0;
    virtual ~IFileSysBldr() {}
};
extern "C" QSharedPointer<IFileSysBldr> IFileSysBldrCtr();
typedef QSharedPointer<IFileSysBldr> (IFileSysBldrCtr_t)();
typedef std::function<IFileSysBldrCtr_t> f_IFileSysBldrCtr;


// interface IUrlBuilder
// return main user page, album urls by user id and name
struct IUrlBuilder
{
    virtual QString GetMainUserPageUrlByName(const QString& strUserName) = 0;
    virtual QString GetMainUserPageUrlById(const QString& strUserName) = 0;
    virtual QString GetPicUrlByPicId(const QString& strPicId) = 0;
    virtual QString GetCommonAlbumUrlById(const QString& strId) = 0;
    virtual QString GetUserIdFromUrl(const QString& strUrl) = 0;
    virtual QString GetUserNameFromUrl(const QString& strUrl) = 0;
    virtual QString GetPicIdFromUrl(const QString& strUrl) = 0;
    virtual ~IUrlBuilder() {}
};
extern "C" QSharedPointer<IUrlBuilder> IUrlBuilderCtr();
typedef QSharedPointer<IUrlBuilder> (IUrlBuilderCtr_t)();
typedef std::function<IUrlBuilderCtr_t> f_IUrlBuilderCtr;


// interface IHtmlPageElm common for user main page, album, etc
struct IHtmlPageElm
{
    virtual bool IsSelfNamePresent() = 0;
    virtual QString GetPersonalUserLink() = 0;
    virtual QString GetUserName() = 0;
    virtual QString GetUserIdPicPage() = 0;
    virtual QString GetLastActivityTime() = 0;
    virtual QString GetFirstCommonAlbumUrl() = 0;
    virtual QString GetNextCommonAlbumUrl(const QString &strCurAlbumUrl) = 0;
    virtual QStringList GetPicPageUrlsList() = 0;
    virtual QStringList GetPicPageUrlsListByImageIdOnly() = 0;

    virtual QString GetBestPossibleDirectPicUrl() = 0;
    virtual QString GetShownInBrowserDirectPicUrl() = 0;

    virtual int GetTotalPhotoCount_Method1() = 0;
    virtual int GetTotalPhotoCount_Method2() = 0;
    virtual int GetHidePhotoCount() = 0;
    virtual int GetViewLimitedPhotoCount() = 0;

    virtual ~IHtmlPageElm() {}
};
extern "C" QSharedPointer<IHtmlPageElm> IHtmlPageElmCtr(const QString &strHtml);
typedef QSharedPointer<IHtmlPageElm> (IHtmlPageElmCtr_t)(const QString &strHtm);
typedef std::function<IHtmlPageElmCtr_t> f_IHtmlPageElmCtr;
