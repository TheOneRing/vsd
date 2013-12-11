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

#include "vsdchildprocess.h"
#include "vsdprocess.h"

#include <windows.h>
#include <winbase.h>

#include <iostream>
#include <string>
#include <sstream>

using namespace libvsd;


#define VSD_BUFLEN 4096


VSDChildProcess::VSDChildProcess(VSDClient *client, const unsigned long id, const HANDLE fileHandle) :
    m_client(client),
    m_id(id),
    m_handle(OpenProcess(PROCESS_ALL_ACCESS, FALSE, id)),
    m_startTime(std::chrono::high_resolution_clock::now()),
    m_exitCode(STILL_ACTIVE)
{
    wchar_t buff[VSD_BUFLEN];
    GetFinalPathNameByHandle(fileHandle, buff, VSD_BUFLEN, FILE_NAME_OPENED);
    m_path = (buff + 4);
    m_name = m_path.substr(m_path.find_last_of('\\') + 1);
    m_name = m_name.substr(0, m_name.length() - 4);
}

VSDChildProcess::~VSDChildProcess()
{
    CloseHandle(m_handle);
}

const std::chrono::system_clock::duration VSDChildProcess::time() const
{
    if (m_exitCode != STILL_ACTIVE)
    {
        return m_duration;
    }
    return  std::chrono::high_resolution_clock::now() - m_startTime;
}

void VSDChildProcess::processStopped(const int exitCode)
{
    m_duration = std::chrono::high_resolution_clock::now() - m_startTime;
    m_exitCode = exitCode;
}

void VSDChildProcess::processDied(const int exitCode, const int errorCode)
{

    processStopped(exitCode);

    LPVOID error = NULL;
    size_t len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_FROM_SYSTEM |
                               FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL,
                               errorCode,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               (LPWSTR)&error,
                               0, NULL);
    m_error = std::wstring((wchar_t*)error, len);
    LocalFree(error);
}

void VSDChildProcess::processDied(const int exitCode, std::wstring error)
{
    processStopped(exitCode);
    m_error = error;
}

void VSDChildProcess::stop()
{
    if (m_exitCode == STILL_ACTIVE)
    {
        std::wstringstream ws;
        ws << "Killing " << path() << " subprocess" << std::endl;
        m_client->writeErr(ws.str());
        TerminateProcess(handle(), 0);
    }
}
