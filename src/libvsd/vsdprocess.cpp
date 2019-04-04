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



#include "vsdprocess.h"
#include "vsdchildprocess.h"
#include "vsdpipe.h"
#include "utils.h"

#include "ceee/gflag_utils.h"

#include <windows.h>
#include <winbase.h>
#include <iostream>
#include <sstream>


#include <stdlib.h>
#include <map>
#include <time.h>
#include <shlwapi.h>

#include <functional>

using namespace libvsd;

static HANDLE SHUTDOWN_EVENT = CreateEvent(nullptr, false, false, L"Shutdown Event");

class VSDProcess::PrivateVSDProcess{
public:

    PrivateVSDProcess(const std::wstring &program, const std::wstring &arguments, VSDClient *client) :
        m_client(client),
        m_program(program),
        m_arguments(arguments)
    {
        if (m_program.length() < 4 || m_program.compare(m_program.length() - 4, 4, L".exe", 4))
        {
            m_program.append(L".exe");
        }

        if (PathFileExists(m_program.data()))
        {
            auto size = GetFullPathName(m_program.data(), 0, nullptr, nullptr);
            // size with null
            if (size != (m_program.size() + 1)) {
                // we can't modify it inplace
                const auto copy = m_program;
                m_program.resize(size + 1);
                GetFullPathName(copy.data(), size, m_program.data(), nullptr);
            }
        }
        else
        {
            m_program.resize(MAX_PATH);
            // PathFindOnPath will modify the content so we need to resize
            if (!PathFindOnPathW(m_program.data(), nullptr))
            {
                std::wstringstream ws;
                ws << "Couldn't find " << m_program << std::endl;
                m_client->writeErr(ws.str());
                return;
            }
        }
        Utils::trimNull(m_program);
    }

    ~PrivateVSDProcess()
    {
    }

    inline void readDebugMSG(DEBUG_EVENT &debugEvent){
        VSDChildProcess *child = m_children[debugEvent.dwProcessId];
        const OUTPUT_DEBUG_STRING_INFO  &DebugString = debugEvent.u.DebugString;

        // a std::string always appends the 0 character,
        // reading the full string would result and a 0 as part of the string
        const size_t size = DebugString.nDebugStringLength - 1;

        std::wstring out;
        if (DebugString.fUnicode == TRUE)
        {
            out.resize(size/2);
            ReadProcessMemory(child->handle(), DebugString.lpDebugStringData, out.data(), size, nullptr);
        }
        else
        {
            std::string tmp(size, 0);
            ReadProcessMemory(child->handle(), DebugString.lpDebugStringData, tmp.data(), size, nullptr);
            out = Utils::multiByteToWideChar(tmp);
        }

        m_client->writeDebug(child, out);
    }

    inline void readProcessCreated(DEBUG_EVENT &debugEvent){
        VSDChildProcess *child = new VSDChildProcess(m_client, debugEvent.dwProcessId, debugEvent.u.CreateProcessInfo.hFile);
        m_children[debugEvent.dwProcessId] = child;
        m_client->processStarted(child);
    }

    inline void cleanup(VSDChildProcess *child, DEBUG_EVENT &debugEvent)
    {
        m_client->processStopped(child);
        m_children.erase(child->id());
        if(m_pi.dwProcessId == child->id())
        {
            m_exitCode = debugEvent.u.ExitProcess.dwExitCode;
            m_time = child->time();

            for (auto it : m_children)//first stop everything and then cleanup
            {
                it.second->stop();
            }
            readOutput(m_stdout);
            readOutput(m_stderr);
        }
        delete child;
    }

    inline void readProcessExited(DEBUG_EVENT &debugEvent){
        VSDChildProcess *child = m_children[debugEvent.dwProcessId];
        child->processStopped(debugEvent.u.ExitProcess.dwExitCode);
        cleanup(child, debugEvent);
    }

    inline void dllLoadEvent(DEBUG_EVENT &debugEvent){
        VSDChildProcess *child = m_children.at(debugEvent.dwProcessId);
        const LOAD_DLL_DEBUG_INFO  &DebugDll = debugEvent.u.LoadDll;
        std::wstring out = L"Unknown";
        if (DebugDll.hFile && DebugDll.hFile != INVALID_HANDLE_VALUE) {
            const auto it = child->m_dllNames.find(DebugDll.lpBaseOfDll);
            if (it == child->m_dllNames.cend())
            {
                out = Utils::getFinalPathNameByHandle(DebugDll.hFile);
                child->m_dllNames.emplace(DebugDll.lpBaseOfDll, out);
            }
            else
            {
                out = it->second;
            }

            CloseHandle(DebugDll.hFile);
        }
        m_client->writeDllLoad(child, out, true);
    }


    inline void dllUnloadEvent(DEBUG_EVENT &debugEvent){
        VSDChildProcess *child = m_children.at(debugEvent.dwProcessId);
        const UNLOAD_DLL_DEBUG_INFO  &DebugDll = debugEvent.u.UnloadDll;
        std::wstring out = L"Unknown";
        const auto dllName = child->m_dllNames.find(DebugDll.lpBaseOfDll);
        if (dllName != child->m_dllNames.cend()) {
            out = dllName->second;
        }
        m_client->writeDllLoad(child, out, false);
    }

#define exeptionString(x) case x : out << #x;break
    inline DWORD readException(DEBUG_EVENT &debugEvent){
        VSDChildProcess *child = m_children[debugEvent.dwProcessId];

        if (debugEvent.u.Exception.dwFirstChance == 0)
        {
            std::wstringstream out;
            out << L"Unhandled Exception: ";
            switch (debugEvent.u.Exception.ExceptionRecord.ExceptionCode)
            {
            exeptionString(EXCEPTION_ACCESS_VIOLATION);
            exeptionString(EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
            exeptionString(EXCEPTION_BREAKPOINT);
            exeptionString(EXCEPTION_DATATYPE_MISALIGNMENT);
            exeptionString(EXCEPTION_FLT_DENORMAL_OPERAND);
            exeptionString(EXCEPTION_FLT_DIVIDE_BY_ZERO);
            exeptionString(EXCEPTION_FLT_INEXACT_RESULT);
            exeptionString(EXCEPTION_FLT_INVALID_OPERATION);
            exeptionString(EXCEPTION_FLT_OVERFLOW);
            exeptionString(EXCEPTION_FLT_STACK_CHECK);
            exeptionString(EXCEPTION_FLT_UNDERFLOW);
            exeptionString(EXCEPTION_ILLEGAL_INSTRUCTION);
            exeptionString(EXCEPTION_IN_PAGE_ERROR);
            exeptionString(EXCEPTION_INT_DIVIDE_BY_ZERO);
            exeptionString(EXCEPTION_INT_OVERFLOW);
            exeptionString(EXCEPTION_INVALID_DISPOSITION);
            exeptionString(EXCEPTION_NONCONTINUABLE_EXCEPTION);
            exeptionString(EXCEPTION_PRIV_INSTRUCTION);
            exeptionString(EXCEPTION_SINGLE_STEP);
            exeptionString(EXCEPTION_STACK_OVERFLOW);
            default:
                out << debugEvent.u.Exception.ExceptionRecord.ExceptionCode;
            }
            child->processDied(debugEvent.u.ExitProcess.dwExitCode, out.str());
            //dont delete child !!
        }
        return DBG_EXCEPTION_NOT_HANDLED;
    }

    inline void readProcessRip(DEBUG_EVENT &debugEvent){
        VSDChildProcess *child = m_children[debugEvent.dwProcessId];
        child->processDied(debugEvent.u.ExitProcess.dwExitCode, debugEvent.u.RipInfo.dwError);
        cleanup(child, debugEvent);
    }

    inline void readOutput(VSDPipe *p)
    {
        if(p)
        {
            BOOL bSuccess = FALSE;
            DWORD dwRead;
            bSuccess = PeekNamedPipe(p->hRead, nullptr, 0, nullptr, &dwRead, nullptr);
            if (bSuccess && dwRead > 0)
            {
                std::string tmp(dwRead, 0);
                if (ReadFile(p->hRead, tmp.data(), dwRead, nullptr, &p->overlapped))
                {
                    std::wstring out = Utils::multiByteToWideChar(tmp);

                    if (p == m_stdout)
                    {
                        m_client->writeStdout(out);
                    }
                    else
                    {
                        m_client->writeErr(out);
                    }

                }
            }
        }
    }

    int run(VSDProcess::ProcessChannelMode channelMode)
    {
        if (m_program.empty())
        {
            return -1;
        }

        SECURITY_ATTRIBUTES sa = {};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;

        m_si.cb = sizeof(m_si);
        m_si.dwFlags |= STARTF_USESTDHANDLES;

        m_stdout = new VSDPipe(&sa);
        if (!m_stdout->isValid())
        {
            std::wcerr << L"Failed to setup pipe for stdout." << std::endl;
            exit(1);
        }

        m_si.hStdOutput = m_stdout->hWrite;
        m_si.hStdError = m_stdout->hWrite;

        if(channelMode != VSDProcess::MergedChannels)
        {
            m_stderr = new VSDPipe(&sa);
            if (!m_stderr->isValid())
            {
                std::wcerr << L"Failed to setup pipe for stderr." << std::endl;
                exit(1);
            }
            m_si.hStdError = m_stderr->hWrite;
        }

        const unsigned long debugConfig = m_debugSubProcess ? DEBUG_ONLY_THIS_PROCESS : DEBUG_PROCESS;

        std::wstringstream tmp;
        tmp << "\"" << m_program << "\" " << m_arguments;
        HRESULT hr = testing::CreateProcessWithGFlags(m_program.data(), tmp.str().data(), nullptr, nullptr, TRUE, debugConfig, nullptr, nullptr, &m_si, &m_pi, m_gFlags);
        if (!SUCCEEDED(hr))
        {
            std::wstringstream ws;
            ws << "Failed to start " << m_program << " " << m_arguments << " " << std::hex << hr << std::dec << Utils::formatError(hr) << std::endl;
            m_client->writeErr(ws.str());
            return -1;
        }

        DEBUG_EVENT debug_event = {};
        DWORD status = DBG_CONTINUE;


        typedef BOOL (WINAPI *debug_wait)(LPDEBUG_EVENT, DWORD);

        const auto kernel = LoadLibrary(L"Kernel32.dll");
        auto waitForDebug = (debug_wait)GetProcAddress(kernel, "WaitForDebugEventEx");
        if (!waitForDebug) {
          waitForDebug = (debug_wait)GetProcAddress(kernel, "WaitForDebugEvent");
        }
        FreeLibrary(kernel);

        do{
            status = DBG_CONTINUE;
            readOutput(m_stdout);
            readOutput(m_stderr);
            if ((*waitForDebug)(&debug_event, 500)){
                readOutput(m_stdout);
                readOutput(m_stderr);
                switch (debug_event.dwDebugEventCode){
                case  OUTPUT_DEBUG_STRING_EVENT:
                    readDebugMSG(debug_event);
                    break;
                case CREATE_PROCESS_DEBUG_EVENT:
                    readProcessCreated(debug_event);
                    break;
                case EXIT_PROCESS_DEBUG_EVENT:
                    readProcessExited(debug_event);
                    break;
                case RIP_EVENT:
                    readProcessRip(debug_event);
                    break;
                case EXCEPTION_DEBUG_EVENT:
                    status = readException(debug_event);
                    break;
                case LOAD_DLL_DEBUG_EVENT:
                    dllLoadEvent(debug_event);
                    break;
                case UNLOAD_DLL_DEBUG_EVENT:
                    dllUnloadEvent(debug_event);
                    break;
                default:
                    break;
                }
            }
            ContinueDebugEvent(debug_event.dwProcessId, debug_event.dwThreadId, status);
        } while (m_children.size() > 0);

        CloseHandle(m_pi.hProcess);
        CloseHandle(m_pi.hThread);
        delete m_stdout;
        m_stdout = nullptr;
        if(m_stderr)
        {
            delete m_stderr;
            m_stderr = nullptr;
        }
        return m_exitCode;
    }

    static BOOL CALLBACK shutdown(HWND hwnd, LPARAM procId)
    {
        DWORD currentProcId = 0;
        GetWindowThreadProcessId(hwnd, &currentProcId);
        if (currentProcId == (DWORD)procId)
        {
            if (SUCCEEDED(PostMessage(hwnd, WM_CLOSE, 0, 0)))
            {
                PostMessage(hwnd, WM_QUIT, 0, 0);
                SetEvent(SHUTDOWN_EVENT);
            }
        }
        return TRUE;
    }

    void stop()
    {

        EnumWindows(shutdown, m_pi.dwProcessId);
        if (WaitForSingleObject(SHUTDOWN_EVENT, 50) != WAIT_OBJECT_0)
        {
            m_client->writeErr(L"Failed to post WM_CLOSE message\n");
            m_children[m_pi.dwProcessId]->stop();
            return;
        }
        if (FAILED(PostThreadMessage(m_pi.dwThreadId, WM_CLOSE, 0, 0)) || FAILED(PostThreadMessage(m_pi.dwThreadId, WM_QUIT, 0, 0)))
        {
            m_client->writeErr(L"Failed to post thred message\n");
        }

        if (WaitForSingleObject(m_pi.hProcess, 10000) == WAIT_TIMEOUT)
        {
            m_children[m_pi.dwProcessId]->stop();
        }
    }



    VSDClient *m_client;
    std::wstring m_program;
    std::wstring m_arguments;
    bool m_debugSubProcess = false;
    unsigned long m_gFlags = {};

    unsigned long m_exitCode = STILL_ACTIVE;
    std::chrono::high_resolution_clock::duration m_time;

    STARTUPINFO m_si = {};
    PROCESS_INFORMATION m_pi = {};
    VSDPipe *m_stdout = nullptr;
    VSDPipe *m_stderr = nullptr;

    std::map <unsigned long, VSDChildProcess*> m_children;
};

VSDClient::VSDClient()
{

}

VSDClient::~VSDClient()
{

}

VSDProcess::VSDProcess(const std::wstring &program, const std::wstring &arguments, VSDClient *client)
    :d(new PrivateVSDProcess(program, arguments, client))
{

}

VSDProcess::~VSDProcess()
{
    delete d;
}

int VSDProcess::run(VSDProcess::ProcessChannelMode channelMode)
{
    return d->run(channelMode);
}

void VSDProcess::stop()
{
    d->stop();
}


void VSDProcess::debugSubProcess(bool b)
{
    d->m_debugSubProcess = b;
}

void VSDProcess::debugDllLoading(bool b)
{
    if (b) {
        d->m_gFlags |= FLG_SHOW_LDR_SNAPS;
    } else {
        d->m_gFlags &= ~FLG_SHOW_LDR_SNAPS;
    }
}

const std::wstring &VSDProcess::program() const
{
    return d->m_program;
}

const std::wstring &VSDProcess::arguments() const
{
    return d->m_arguments;
}

int VSDProcess::exitCode() const
{
    return d->m_exitCode;
}

const std::chrono::high_resolution_clock::duration &VSDProcess::time() const
{
    return d->m_time;
}

