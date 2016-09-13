// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include <string>
#include <assert.h>
#include <functional>
#include <exception>
#include <memory>
#include <list>

#ifdef QT_VERSION
inline QString QsFrWs(const std::wstring &wstr)
{
    return QString::fromStdWString(wstr);
}
#endif

#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))


#ifdef _WIN32
    #define DECL_EXPORT __declspec(dllexport)
    #define DECL_IMPORT __declspec(dllimport)
#else
    #define DECL_EXPORT
    #define DECL_IMPORT
#endif

#if defined(PICOLLECTION_LIB)
#  define SHARED_EXPORT DECL_EXPORT
#else
#  define SHARED_EXPORT DECL_IMPORT
#endif


class parse_ex: public std::exception
{
    std::wstring m_strWhat, m_strDetails;
public:
    parse_ex(const std::wstring &strWhat, const std::wstring &strDetails)
        : m_strWhat(strWhat), m_strDetails(strDetails) { }
    // const char* what() const noexcept { return m_strWhat.toUtf8(); }
    const std::wstring &strWhat() const noexcept { return m_strWhat; }
    const std::wstring &strDetails() const noexcept { return m_strDetails; }
};


// interface ISiteInfo
// return site related constants (host, login, etc)
struct ISiteInfo
{
    virtual const std::wstring &GetHostName() const = 0;
    virtual const std::wstring &GetProtocol() const = 0;
    virtual const std::wstring &GetProtocolHostName() const = 0;

    virtual const std::wstring &GetLogin() const = 0;
    virtual const std::wstring &GetAuthInfo() const = 0;

    virtual const std::wstring &GetDBFileName() const = 0;
    virtual const std::wstring &GetDBTableName() const = 0;

    virtual bool UsersIdAndNameSame() const = 0;
    virtual bool IsPagePicUrl(const std::wstring &url) const = 0;
    virtual bool IsDirectPicUrl(const std::wstring &url) const = 0;

    virtual ~ISiteInfo() {}
};
extern "C" std::shared_ptr<ISiteInfo> ISiteInfoCtr();
typedef std::shared_ptr<ISiteInfo> (ISiteInfoCtr_t)();
typedef std::function<ISiteInfoCtr_t> f_ISiteInfoCtr;


// TODO: Add const to methods(not only here)
// interface IFileSysBldr some function to build file name
struct IFileSysBldr
{
    virtual std::wstring GetPicFileNameWoExt(
            const std::wstring& strUserId,
            const std::wstring& strFileId) const = 0;

    virtual std::wstring GetUserId(const std::wstring& strFileName) const = 0;
    virtual ~IFileSysBldr() {}
};
extern "C" std::shared_ptr<IFileSysBldr> IFileSysBldrCtr();
typedef std::shared_ptr<IFileSysBldr> (IFileSysBldrCtr_t)();
typedef std::function<IFileSysBldrCtr_t> f_IFileSysBldrCtr;


// interface IUrlBuilder
// return main user page, album urls by user id and name
struct IUrlBuilder
{
    virtual std::wstring GetMainUserPageUrlByName(
            const std::wstring& strUserName) const = 0;
    virtual std::wstring GetMainUserPageUrlById(
            const std::wstring& strUserName) const = 0;
    virtual std::wstring GetPicUrlByPicId(
            const std::wstring& strPicId) const = 0;
    virtual std::wstring GetCommonAlbumUrlById(
            const std::wstring& strId) const = 0;
    virtual std::wstring GetUserIdFromUrl(const std::wstring& strUrl) const = 0;
    virtual std::wstring GetUserNameFromUrl(
            const std::wstring& strUrl) const = 0;
    virtual std::wstring GetPicIdFromUrl(const std::wstring& strUrl) const = 0;
    virtual ~IUrlBuilder() {}
};
extern "C" std::shared_ptr<IUrlBuilder> IUrlBuilderCtr();
typedef std::shared_ptr<IUrlBuilder> (IUrlBuilderCtr_t)();
typedef std::function<IUrlBuilderCtr_t> f_IUrlBuilderCtr;


// interface IHtmlPageElm common for user main page, album, etc
struct IHtmlPageElm
{
    virtual bool IsSelfNamePresent() const = 0;
    virtual std::wstring GetUserName() const = 0;
    virtual std::wstring GetUserIdPicPage() const = 0;
    virtual std::wstring GetLastActivityTime() const = 0;
    virtual std::wstring GetFirstCommonAlbumUrl() const = 0;
    virtual std::wstring GetNextCommonAlbumUrl(
            const std::wstring &strCurAlbumUrl, int iPicOnPageCnt) = 0;
    virtual std::list<std::wstring> GetPicPageUrlsList() const = 0;

    virtual std::wstring GetBestPossibleDirectPicUrl() const = 0;
    virtual std::wstring GetShownInBrowserDirectPicUrl() const = 0;

    virtual int GetTotalPhotoCount() = 0;
    virtual int GetHidePhotoCount() = 0;
    virtual int GetViewLimitedPhotoCount() = 0;

    virtual ~IHtmlPageElm() {}
};
extern "C" std::shared_ptr<IHtmlPageElm> IHtmlPageElmCtr(
        const std::wstring &strHtml);
typedef std::shared_ptr<IHtmlPageElm> (IHtmlPageElmCtr_t)(
        const std::wstring &strHtm);
typedef std::function<IHtmlPageElmCtr_t> f_IHtmlPageElmCtr;


extern "C" SHARED_EXPORT std::shared_ptr<IFileSysBldr> IFileSysBldrCtr();
extern "C" SHARED_EXPORT std::shared_ptr<IUrlBuilder> IUrlBuilderCtr();
extern "C" SHARED_EXPORT std::shared_ptr<ISiteInfo> ISiteInfoCtr();
