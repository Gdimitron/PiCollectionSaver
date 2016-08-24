// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkCookie>

class HttpDownloader : public QObject
{
    Q_OBJECT

public:
    HttpDownloader(QObject *parent);
    ~HttpDownloader();

private:
    QNetworkAccessManager *m_pnam;
    QString m_authinfo;
    QList<QNetworkCookie> m_authCookies;
    QString m_strLastRedirectUrl;
    QByteArray m_arrLastReadAll;

    QNetworkReply* Download(const QUrl&);

public:
    void ObtainAuthCookie(const QString& strUrl, const QString& authifno);

    QNetworkReply *DownloadAsync(const QUrl&);
    QByteArray DownloadSync(const QUrl&);

    QString GetLastRedirectUrl();

signals:
    void downloadProgress(qint64, qint64);
    void done(int, const QUrl&, const QByteArray&);
    void error(const QUrl&, const QString& err);

private slots:
    void slotFinished(QNetworkReply*);
};
