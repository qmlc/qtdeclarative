/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlmemoryprofiler_p.h"
#include <QUrl>

QT_BEGIN_NAMESPACE

enum LibraryState
{
    Unloaded,
    Failed,
    Loaded
};

static LibraryState state = Unloaded;

typedef void (qmlmemprofile_stats)(int *allocCount, int *bytesAllocated);
typedef void (qmlmemprofile_clear)();
typedef void (qmlmemprofile_enable)();
typedef void (qmlmemprofile_disable)();
typedef void (qmlmemprofile_push_location)(const char *filename, int lineNumber);
typedef void (qmlmemprofile_pop_location)();
typedef void (qmlmemprofile_save)(const char *filename);
typedef int (qmlmemprofile_is_enabled)();

static qmlmemprofile_stats *memprofile_stats;
static qmlmemprofile_clear *memprofile_clear;
static qmlmemprofile_enable *memprofile_enable;
static qmlmemprofile_disable *memprofile_disable;
static qmlmemprofile_push_location *memprofile_push_location;
static qmlmemprofile_pop_location *memprofile_pop_location;
static qmlmemprofile_save *memprofile_save;
static qmlmemprofile_is_enabled *memprofile_is_enabled;

extern QFunctionPointer qt_linux_find_symbol_sys(const char *symbol);

static bool openLibrary()
{
#if 0//def Q_OS_LINUX   [Disabled until qt_linux_find_symbol_sys is available in qtbase]
    if (state == Unloaded) {
        memprofile_stats = (qmlmemprofile_stats *) qt_linux_find_symbol_sys("qmlmemprofile_stats");
        memprofile_clear = (qmlmemprofile_clear *) qt_linux_find_symbol_sys("qmlmemprofile_clear");
        memprofile_enable = (qmlmemprofile_enable *) qt_linux_find_symbol_sys("qmlmemprofile_enable");
        memprofile_disable = (qmlmemprofile_disable *) qt_linux_find_symbol_sys("qmlmemprofile_disable");
        memprofile_push_location = (qmlmemprofile_push_location *) qt_linux_find_symbol_sys("qmlmemprofile_push_location");
        memprofile_pop_location = (qmlmemprofile_pop_location *) qt_linux_find_symbol_sys("qmlmemprofile_pop_location");
        memprofile_save = (qmlmemprofile_save *) qt_linux_find_symbol_sys("qmlmemprofile_save");
        memprofile_is_enabled = (qmlmemprofile_is_enabled *) qt_linux_find_symbol_sys("qmlmemprofile_is_enabled");

        if (memprofile_stats && memprofile_clear && memprofile_enable && memprofile_disable &&
            memprofile_push_location && memprofile_pop_location && memprofile_save && memprofile_is_enabled)
            state = Loaded;
        else
            state = Failed;
    }
#endif  // Q_OS_LINUX

    return state == Loaded;
}

QQmlMemoryScope::QQmlMemoryScope(const QUrl &url) : pushed(false)
{
    if (openLibrary() && memprofile_is_enabled()) {
        const char *location = url.path().toUtf8().constData();
        memprofile_push_location(location, 0);
        pushed = true;
    }
}

QQmlMemoryScope::QQmlMemoryScope(const char *string) : pushed(false)
{
    if (openLibrary() && memprofile_is_enabled()) {
        memprofile_push_location(string, 0);
        pushed = true;
    }
}

QQmlMemoryScope::~QQmlMemoryScope()
{
    if (pushed)
        memprofile_pop_location();
}

bool QQmlMemoryProfiler::isEnabled()
{
    if (openLibrary())
        return memprofile_is_enabled();

    return false;
}

void QQmlMemoryProfiler::enable()
{
    if (openLibrary())
        memprofile_enable();
}

void QQmlMemoryProfiler::disable()
{
    if (openLibrary())
        memprofile_disable();
}

void QQmlMemoryProfiler::clear()
{
    if (openLibrary())
        memprofile_clear();
}

void QQmlMemoryProfiler::stats(int *allocCount, int *bytesAllocated)
{
    if (openLibrary())
        memprofile_stats(allocCount, bytesAllocated);
}

void QQmlMemoryProfiler::save(const char *filename)
{
    if (openLibrary())
        memprofile_save(filename);
}

QT_END_NAMESPACE