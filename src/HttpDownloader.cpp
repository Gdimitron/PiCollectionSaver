// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "HttpDownloader.h"
#include <QEventLoop>
#include <QTimer>

HttpDownloader::HttpDownloader(QObject *parent)
    : QObject(parent)
{
    m_pnam = new QNetworkAccessManager(this);

    connect(m_pnam, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(slotFinished(QNetworkReply*)));
}

HttpDownloader::~HttpDownloader()
{
}

void HttpDownloader::ObtainAuthCookie(const QUrl& url, const QString& authinfo)
{
    // uploading file using post on Qt: http://codepaste.ru/6479/
    QNetworkRequest request("http://" + url.host());

    m_authinfo = authinfo;
    Q_ASSERT(!m_authinfo.isEmpty());
    QByteArray byteArr(m_authinfo.toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");
    QNetworkReply* pnr = m_pnam->post(request, byteArr);

    while(m_authCookies.isEmpty()) {
        // wait for cookies
        QEventLoop loop;
        QTimer::singleShot(200/*ms*/, &loop, SLOT(quit()));
        loop.exec();
    }
}

QNetworkReply* HttpDownloader::Download(const QUrl& url)
{
    // some info about cookie -
    // http://developer.nokia.com/Community/Wiki/How_to_use_http_cookies_with_Qt
    QNetworkRequest request(url);

    if (!m_authCookies.isEmpty()) {
        QVariant varCookie;
        varCookie.setValue(m_authCookies);
        request.setHeader(QNetworkRequest::CookieHeader, varCookie);
    }

    QNetworkReply* pNetReply = m_pnam->get(request);
    return pNetReply;

    /*connect(pnr, SIGNAL(downloadProgress(qint64, qint64)),
        this, SIGNAL(downloadProgress(qint64, qint64)));*/
}

QNetworkReply* HttpDownloader::DownloadAsync(const QUrl& url)
{
    return Download(url);
}

QByteArray HttpDownloader::DownloadSync(const QUrl& url)
{
    m_arrLastReadAll.clear();

    Download(url);

    QEventLoop loopFinished;
    connect(this, SIGNAL(done(int, const QUrl&, const QByteArray&)),
            &loopFinished, SLOT(quit()));
    connect(this, SIGNAL(error(const QUrl&, const QString&)),
            &loopFinished, SLOT(quit()));
    loopFinished.exec();

    disconnect(this, 0, &loopFinished, 0);

    return m_arrLastReadAll;
}

QString HttpDownloader::GetLastRedirectUrl()
{
    return m_strLastRedirectUrl;
}

void HttpDownloader::slotFinished(QNetworkReply* pnr)
{
    if (pnr->error() != QNetworkReply::NoError) {
        emit error(pnr->url(), pnr->errorString());
    } else {
        int iReplCode = pnr->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute).toInt();
        switch(iReplCode)
        {
        case 302: // redirect
            m_strLastRedirectUrl = pnr->attribute(
                        QNetworkRequest::RedirectionTargetAttribute).toString();
            this->Download(pnr->attribute(
                        QNetworkRequest::RedirectionTargetAttribute).toUrl());
            break;

        case 200:
            if (!m_authinfo.isEmpty() && m_authCookies.isEmpty()) {
                pnr->readAll();
                //QList<QByteArray> headers = pnr->rawHeaderList();
                QVariant cookie = pnr->header(QNetworkRequest::SetCookieHeader);
                m_authCookies = qvariant_cast<QList<QNetworkCookie>>(cookie);
            } else {
                m_arrLastReadAll = pnr->readAll();
                emit done(iReplCode, pnr->url(), m_arrLastReadAll);
            }
            break;

        default:
            Q_ASSERT(false);
        }
    }
    pnr->deleteLater();
}
