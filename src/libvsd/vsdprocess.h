/*
    VSD prints debugging messages of applications and their
    sub-processes to console and supports logging of their output.
    Copyright (C) 2012-2013  Patrick von Reth <vonreth@kde.org>


    VSD is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VSD is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with VSD.  If not, see <http://www.gnu.org/licenses/>.
    */

#ifndef VSDPROCESS_H
#define VSDPROCESS_H

#include "vsd_exports.h"

#include <windows.h>
#include <string>
#include <sstream>
#include <chrono>

namespace libvsd {

class VSDChildProcess;

class LIBVSD_EXPORT VSDClient
{
public:
    VSDClient();
    virtual ~VSDClient();
    virtual void writeStdout(const std::wstring &data) = 0;
    virtual void writeErr(const std::wstring &data) = 0;
    virtual void writeDebug(const VSDChildProcess *process, const std::wstring &data) = 0;
    virtual void processStarted(const VSDChildProcess* process) = 0;
    virtual void processStopped(const VSDChildProcess* process) = 0;
};

class LIBVSD_EXPORT VSDProcess
{
public:
    enum ProcessChannelMode
    {
        SeperateChannels,
        MergedChannels

    };

    VSDProcess(const std::wstring &program, const std::wstring &arguments, VSDClient *client);
    virtual ~VSDProcess();

    int run(VSDProcess::ProcessChannelMode channelMode =  VSDProcess::MergedChannels);
    void stop();
    void debugSubProcess(bool b);
    const std::wstring &program() const;
    const std::wstring &arguments() const;
    int exitCode() const;
    const std::chrono::high_resolution_clock::duration &time() const;

private:
    class PrivateVSDProcess;
    PrivateVSDProcess *d;

};
}

#endif
