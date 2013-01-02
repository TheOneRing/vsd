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

#include "vsdchildprocess.h"
#include "vsdprocess.h"

#include <windows.h>
#include <winbase.h>

#include <iostream>
#include <string>
#include <sstream>

using namespace libvsd;

namespace cr = std::chrono;

#define VSD_BUFLEN 4096


VSDChildProcess::VSDChildProcess(VSDClient *client, const unsigned long id, const HANDLE fileHandle) :
    m_client(client),
    m_id(id),
    m_handle(OpenProcess(PROCESS_ALL_ACCESS, FALSE, id)),
    m_startTime(std::chrono::high_resolution_clock::now()),
    m_exitCode(STILL_ACTIVE)
{
    wchar_t buff[VSD_BUFLEN];
    GetFinalPathNameByHandle(fileHandle,buff,VSD_BUFLEN,FILE_NAME_OPENED);
    m_path = (buff+4);
    m_name = m_path.substr(m_path.find_last_of('\\')+1);
}

VSDChildProcess::~VSDChildProcess()
{
    CloseHandle(m_handle);
}

const HANDLE &VSDChildProcess::handle() const
{
    return m_handle;
}

const std::wstring &VSDChildProcess::path() const
{
    return m_path;
}

const std::wstring &VSDChildProcess::name() const
{
    return m_name;
}

const std::chrono::system_clock::duration VSDChildProcess::time() const
{
    if(m_exitCode != STILL_ACTIVE)
        return m_duration;
    return  std::chrono::high_resolution_clock::now() - m_startTime;
}

const unsigned long VSDChildProcess::id() const
{
    return m_id;
}

const int VSDChildProcess::exitCode() const
{
    return m_exitCode;
}

void VSDChildProcess::processStopped(const int exitCode)
{
    m_exitCode = exitCode;
    m_duration = std::chrono::high_resolution_clock::now() - m_startTime;
}

void VSDChildProcess::stop()
{
    if(m_exitCode == STILL_ACTIVE)
    {
        std::wstringstream ws;
        ws<<"Killing "<<path()<<" subprocess"<<std::endl;
        m_client->writeErr(ws.str());
        TerminateProcess(handle(), 0);
    }
}
