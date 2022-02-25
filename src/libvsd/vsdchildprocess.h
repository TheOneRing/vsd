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

#ifndef VSDCHILDPROCESS_H
#define VSDCHILDPROCESS_H

#include "vsd_exports.h"

#include <chrono>
#include <filesystem>
#include <map>
#include <optional>
#include <string>

#include <windows.h>

// needs to be included after windows.h
#include <psapi.h>

namespace libvsd {
class VSDClient;
class VSDChildProcess;

class Module
{
public:
    Module(const LOAD_DLL_DEBUG_INFO &info, VSDChildProcess *parent);

    std::optional<MODULEINFO> info() const;
    const auto &name() const
    {
        return m_name;
    }
    const std::wstring &error() const
    {
        return m_error;
    }

private:
    VSDChildProcess *m_parent = nullptr;
    const HMODULE m_module = nullptr;
    mutable std::wstring m_error;
    mutable MODULEINFO m_info = {};
    const std::filesystem::path m_name;
};

class LIBVSD_EXPORT VSDChildProcess
{
public:
    VSDChildProcess(VSDClient *client, const unsigned long id, const HANDLE fileHandle);
    virtual ~VSDChildProcess();

    inline const HANDLE &handle() const
    {
        return m_handle;
    }

    inline const std::wstring &path() const
    {
        return m_path;
    }

    inline const std::wstring &name() const
    {
        return m_name;
    }

    inline const std::wstring &error() const
    {
        return m_error;
    }

    const std::chrono::high_resolution_clock::duration time() const;

    inline unsigned long id() const
    {
        return m_id;
    }

    inline uint32_t exitCode() const
    {
        return m_exitCode;
    }

    void processStopped(const uint32_t exitCode);

    void processDied(const uint32_t exitCode, const int error);

    void processDied(const uint32_t exitCode, std::wstring error);

    void stop();

    std::optional<Module> getExceptionModule(void *address) const;

    std::optional<Module> addModule(const LOAD_DLL_DEBUG_INFO &info);

    std::optional<Module> getModul(HMODULE baseAddress) const;

private:
#pragma warning(disable : 4251)

    VSDClient *m_client;
    unsigned long m_id;
    HANDLE m_handle;
    std::wstring m_path;
    std::wstring m_name;
    std::wstring m_error;
    std::chrono::high_resolution_clock::time_point m_startTime;
    std::chrono::high_resolution_clock::duration m_duration;

    uint32_t m_exitCode;
    std::map<HMODULE, Module> m_modules;
};

}

#endif // VSDCHILDPROCESS_H
