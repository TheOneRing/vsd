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

#ifndef VSDPROCESS_H
#define VSDPROCESS_H

#include "vsd_exports.h"

#include <windows.h>
#include <sstream>

namespace libvsd {

class VSDChildProcess;

class LIBVSD_EXPORT VSDClient
{
public:
    VSDClient();
    ~VSDClient();
    virtual void writeStdout(const std::wstringstream &data) = 0;
    virtual void writeErr(const std::wstringstream &data) = 0;
    virtual void writeDebug(const VSDChildProcess *process,const std::wstringstream &data) = 0;
    virtual void processStarted(const VSDChildProcess* process) = 0;
    virtual void processStopped(const VSDChildProcess* process) = 0;
};

class LIBVSD_EXPORT VSDProcess
{
public:
    VSDProcess(const wchar_t *program,const wchar_t * arguments,VSDClient *client);
    ~VSDProcess();

    int run();
    void stop();
    void debugSubProcess(bool b);
    const wchar_t *program() const;
    const wchar_t *arguments() const;

private:
    class PrivateVSDProcess;
    PrivateVSDProcess *d;

};
}

#endif
