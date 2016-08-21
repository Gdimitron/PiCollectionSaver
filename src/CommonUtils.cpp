// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "CommonUtils.h"
#include <QTextCodec>

CommonUtils::CommonUtils()
{
}

QString CommonUtils::Win1251ToQstring(const QByteArray& byteArray)
{
    static QTextCodec *m_win1251Codec(QTextCodec::codecForName("Windows-1251"));
    return m_win1251Codec->toUnicode(byteArray);
}
