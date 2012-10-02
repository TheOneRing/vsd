/****************************************************************************
** VSD prints the debug output of a windows GUI apllication to a console window
** Copyright (C) 2012  Patrick von Reth <vonreth@kde.org>
**
**
** VSD is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
* * License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** VSD is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with VSD; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef VSDCHILDPROCESS_H
#define VSDCHILDPROCESS_H

#include "vsd_exports.h"

#include <windows.h>
#include <wchar.h>

namespace libvsd
{

class LIBVSD_EXPORT VSDChildProcess
{
public:
    VSDChildProcess(const unsigned long id,const HANDLE fileHandle);
    ~VSDChildProcess();

    const HANDLE& handle() const;
    const wchar_t *path() const;
    const wchar_t *name() const;
    const double time() const;
    const unsigned long id() const;
    const int exitCode() const;

    void processStopped(const int exitCode);

private:
    long findLastBackslash(const wchar_t *in);

    HANDLE m_handle;
    wchar_t* m_path;
    wchar_t* m_name;
    unsigned long m_id;
    time_t m_startTime;
    double m_duration;
    int m_exitCode;
    bool m_stopped;

};

}

#endif // VSDCHILDPROCESS_H
