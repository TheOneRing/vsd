/*
    VSD prints debugging messages of applications and their
    sub-processes to console and supports logging of their output.
    Copyright (C) 2012-2020  Hannah von Reth <vonreth@kde.org>


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
Module::Module(const LOAD_DLL_DEBUG_INFO &info, VSDChildProcess *parent)
    : m_name(Utils::getFinalPathNameByHandle(info.hFile))
    , m_parent(parent)
    , m_module(static_cast<HMODULE>(info.lpBaseOfDll))
{
}

std::optional<MODULEINFO> libvsd::Module::info() const
{
    // the module needs to ble loaded already
    if (!GetModuleInformation(m_parent->handle(), m_module, &m_info, sizeof(m_info))) {
        m_error = L"(Error: GetModuleInformation: " + Utils::formatError(GetLastError()) + L")";
        return {};
    }
    return m_info;
}

VSDChildProcess::VSDChildProcess(VSDClient *client, const unsigned long id, const HANDLE fileHandle)
    : m_client(client)
    , m_id(id)
    , m_handle(OpenProcess(PROCESS_ALL_ACCESS, FALSE, id))
    , m_path(Utils::getFinalPathNameByHandle(fileHandle))
    , m_name(m_path.stem())
    , m_startTime(std::chrono::high_resolution_clock::now())
    , m_exitCode(STILL_ACTIVE)
{
}

VSDChildProcess::~VSDChildProcess()
{
    CloseHandle(m_handle);
}

const std::chrono::high_resolution_clock::duration VSDChildProcess::time() const
{
    if (m_exitCode != STILL_ACTIVE) {
        return m_duration;
    }
    return std::chrono::high_resolution_clock::now() - m_startTime;
}

void VSDChildProcess::processStopped(const uint32_t exitCode)
{
    m_duration = std::chrono::high_resolution_clock::now() - m_startTime;
    m_exitCode = exitCode;
}

void VSDChildProcess::processDied(const uint32_t exitCode, const int errorCode)
{
    processStopped(exitCode);
    m_error = Utils::formatError(errorCode);
}

void VSDChildProcess::processDied(const uint32_t exitCode, std::wstring error)
{
    processStopped(exitCode);
    m_error = error;
}

void VSDChildProcess::stop()
{
    if (m_exitCode == STILL_ACTIVE) {
        std::wstringstream ws;
        ws << "Killing " << path() << " subprocess" << std::endl;
        m_client->writeErr(ws.str());
        TerminateProcess(handle(), 0);
    }
}


std::optional<Module> VSDChildProcess::getExceptionModule(void *address) const
{
    // find the module that comes after address
    auto tmp = std::find_if(m_modules.cbegin(), m_modules.cend(), [address](const auto &other) {
        const auto info = other.second.info();
        if (!info) {
            return false;
        }
        void *end = reinterpret_cast<char *>(info->lpBaseOfDll) + info->SizeOfImage;
        return address == std::clamp(address, info->lpBaseOfDll, end);
    });
    if (tmp == m_modules.cend()) {
        return {};
    }
    return tmp->second;
}

std::optional<Module> VSDChildProcess::getModul(HMODULE baseAddress) const
{
    const auto it = m_modules.find(baseAddress);
    if (it == m_modules.cend()) {
        return {};
    }
    return it->second;
}

std::optional<Module> VSDChildProcess::addModule(const LOAD_DLL_DEBUG_INFO &info)
{
    const auto module = static_cast<HMODULE>(info.lpBaseOfDll);
    auto out = m_modules.find(module);
    if (info.hFile) {
        if (out == m_modules.cend()) {
            out = m_modules.emplace(module, Module { info, this }).first;
        }
        CloseHandle(info.hFile);
    }
    if (out == m_modules.cend()) {
        return {};
    }
    return out->second;
}
