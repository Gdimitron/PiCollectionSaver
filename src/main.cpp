// Copyright 2016 The PiCollectionSaver Authors. All rights reserved.
// Use of this source code is governed by a GNU V3.0 license that can be
// found in the LICENSE file.

#include "picollectionsaver.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	PiCollectionSaver w;
	w.show();
	return a.exec();
}
