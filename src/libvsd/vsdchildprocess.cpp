/*
    VSD prints debugging messages of applications and their
    sub-processes to console and supports logging of their output.
    Copyright (C) 2012-2019  Hannah von Reth <vonreth@kde.org>


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
#include "utils.h"

#include <windows.h>
#include <winbase.h>

#include <iostream>
#include <string>
#include <sstream>

using namespace libvsd;


VSDChildProcess::VSDChildProcess(VSDClient *client, const unsigned long id, const HANDLE fileHandle) :
    m_client(client),
    m_id(id),
    m_handle(OpenProcess(PROCESS_ALL_ACCESS, FALSE, id)),
    m_path(Utils::getFinalPathNameByHandle(fileHandle)),
    m_startTime(std::chrono::high_resolution_clock::now()),
    m_exitCode(STILL_ACTIVE)
{
    const auto start = m_path.find_last_of(L'\\') + 1;
    m_name = m_path.substr(start, m_path.length() - start - 5);
}

VSDChildProcess::~VSDChildProcess()
{
    CloseHandle(m_handle);
}

const std::chrono::high_resolution_clock::duration VSDChildProcess::time() const
{
    if (m_exitCode != STILL_ACTIVE)
    {
        return m_duration;
    }
    return  std::chrono::high_resolution_clock::now() - m_startTime;
}

void VSDChildProcess::processStopped(const uint32_t exitCode)
{
    m_duration = std::chrono::high_resolution_clock::now() - m_startTime;
    m_exitCode = exitCode;
}

void VSDChildProcess::processDied(const uint32_t exitCode, const int errorCode)
{

    processStopped(exitCode);

    wchar_t *error = nullptr;
    size_t len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_FROM_SYSTEM |
                               FORMAT_MESSAGE_IGNORE_INSERTS,
                               nullptr,
                               errorCode,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               reinterpret_cast<LPWSTR>(&error),
                               0, nullptr);
    m_error = std::wstring(error, len);
    LocalFree(error);
}

void VSDChildProcess::processDied(const uint32_t exitCode, std::wstring error)
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
