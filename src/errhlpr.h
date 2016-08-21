// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#pragma once
#include <QString>

class CErrHlpr
{
public:
    static bool IgnMsgBox(const QString& strText,
                          const QString& strInformativeText);

    static void Critical(const QString& strText,
                         const QString& strDetailedText);

    static void Fatal(const QString& strText, const QString& strDetailedText);

};
