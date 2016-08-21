// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include <QString>

class CommonUtils
{
public:
    CommonUtils();
    static QString Win1251ToQstring(const QByteArray&);
};
