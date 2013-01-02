/*
    VSD prints debugging messages of applications and their
    sub-processes to console and supports logging of their output.
    Copyright (C) 2012  Patrick von Reth <vonreth@kde.org>


    VSD is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VSD is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with SnarlNetworkBridge.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef VSDCHILDPROCESS_H
#define VSDCHILDPROCESS_H

#include "vsd_exports.h"

#include <windows.h>
#include <string>
#include <chrono>

namespace libvsd
{
class VSDClient;

class LIBVSD_EXPORT VSDChildProcess
{
public:
    VSDChildProcess(VSDClient *client, const unsigned long id, const HANDLE fileHandle);
    ~VSDChildProcess();

    const HANDLE& handle() const;
    const std::wstring &path() const;
    const std::wstring &name() const;
    const std::chrono::high_resolution_clock::duration time() const;
    const unsigned long id() const;
    const int exitCode() const;

    void processStopped(const int exitCode);
    void stop();

private:
    VSDClient  *m_client;
    HANDLE m_handle;
    std::wstring m_path;
    std::wstring  m_name;
    unsigned long m_id;
    std::chrono::high_resolution_clock::time_point m_startTime;
    std::chrono::high_resolution_clock::duration m_duration;
    int m_exitCode;


};

}

#endif // VSDCHILDPROCESS_H
