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


#include "vsdprocess.h"
#include "vsdchildprocess.h"

#include <windows.h>
#include <winbase.h>
#include <iostream>
#include <sstream>

#include <stdlib.h>
#include <map>
#include <time.h>

#define VSD_BUFLEN 4096

#ifdef __MINGW64_VERSION_MAJOR
#define PIPE_REJECT_REMOTE_CLIENTS 0x00000008
#endif


//inspired by https://qt.gitorious.org/qt-labs/jom/blobs/master/src/jomlib/process.cpp
using namespace libvsd;

static HANDLE SHUTDOWN_EVENT = CreateEvent(NULL,false,false,L"SHutdown Event");

class VSDProcess::PrivateVSDProcess{
public:
    class Pipe
    {
    public:
        Pipe()
            : hWrite(INVALID_HANDLE_VALUE)
            , hRead(INVALID_HANDLE_VALUE)
        {
            ZeroMemory(&overlapped, sizeof(overlapped));
        }


        ~Pipe()
        {
            if (hWrite != INVALID_HANDLE_VALUE)
                CloseHandle(hWrite);
            if (hRead != INVALID_HANDLE_VALUE)
                CloseHandle(hRead);
        }

        bool operator ==( const Pipe& p) const
        {
            return this->hWrite == p.hWrite && this->hRead == p.hRead;
        }


        HANDLE hWrite;
        HANDLE hRead;
        OVERLAPPED overlapped;
    };

    PrivateVSDProcess(const std::wstring &program,const std::wstring &arguments,VSDClient *client)
        :m_client(client)
        ,m_program(program)
        ,m_arguments(arguments)
        ,m_exitCode(STILL_ACTIVE)
    {

        SECURITY_ATTRIBUTES sa = {0};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;


        if (!setupPipe(m_stdout, &sa))
        {
            std::wcerr<<L"Cannot setup pipe for stdout."<<std::endl;
            exit(1);
        }

        if (!setupPipe(m_stderr, &sa))
        {
            std::wcerr<<L"Cannot setup pipe for stderr."<<std::endl;
            exit(1);
        }

        ZeroMemory( &m_si, sizeof(m_si) );
        m_si.cb = sizeof(m_si);
        m_si.dwFlags |= STARTF_USESTDHANDLES;
        m_si.hStdOutput = m_stdout.hWrite;
        m_si.hStdError =  m_stderr.hWrite;

        ZeroMemory( &m_pi, sizeof(m_pi) );

    }

    ~PrivateVSDProcess()
    {
    }

    bool setupPipe(Pipe &pipe, SECURITY_ATTRIBUTES *sa)
    {
        size_t maxPipeLen = 256;
        wchar_t *pipeName = new wchar_t[maxPipeLen];
        unsigned int randomValue;
        if (rand_s(&randomValue) != 0)
            randomValue = rand();
        swprintf_s(pipeName, maxPipeLen, L"\\\\.\\pipe\\vsd-%X", randomValue);

        DWORD dwPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS;
        const DWORD dwPipeBufferSize = 1024 * 1024;

        pipe.hRead = CreateNamedPipe(pipeName,
                                     PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                                     dwPipeMode,
                                     1,                      // only one pipe instance
                                     0,                      // output buffer size
                                     dwPipeBufferSize,       // input buffer size
                                     0,
                                     sa);
        if(  pipe.hRead == INVALID_HANDLE_VALUE){
            std::wcerr<<L"Creation of the NamedPipe "<<pipeName<<L" failed "<<GetLastError()<<std::endl;
            return false;
        }


        pipe.hWrite = CreateFile(pipeName,
                                 GENERIC_WRITE,
                                 0,
                                 sa,
                                 OPEN_EXISTING,
                                 FILE_FLAG_OVERLAPPED,
                                 NULL);
        if(  pipe.hWrite == INVALID_HANDLE_VALUE){
            std::wcerr<<L"Creation of the pipe "<<pipeName<<L" failed "<<GetLastError()<<std::endl;
            return false;
        }
        ConnectNamedPipe(pipe.hRead, NULL);
        return true;
    }

    void readDebugMSG(DEBUG_EVENT &debugEvent){
        VSDChildProcess *child = m_children[debugEvent.dwProcessId];
        OUTPUT_DEBUG_STRING_INFO  &DebugString = debugEvent.u.DebugString;


        if ( DebugString.fUnicode )
        {
            ReadProcessMemory(child->handle(),DebugString.lpDebugStringData,m_wcharBuffer,DebugString.nDebugStringLength, NULL);
        }
        else
        {
            ReadProcessMemory(child->handle(),DebugString.lpDebugStringData,m_charBuffer,DebugString.nDebugStringLength, NULL);
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, m_charBuffer, -1, m_wcharBuffer, DebugString.nDebugStringLength+1);
        }
        m_client->writeDebug(child,m_wcharBuffer);
    }

    void readProcessCreated(DEBUG_EVENT &debugEvent){
        VSDChildProcess *child = new VSDChildProcess( m_client, debugEvent.dwProcessId, debugEvent.u.CreateProcessInfo.hFile);
        m_children[debugEvent.dwProcessId] = child;
        m_client->processStarted(child);
    }

    void readProcessExited(DEBUG_EVENT &debugEvent){
        VSDChildProcess *child = m_children[debugEvent.dwProcessId];
        child->processStopped(debugEvent.u.ExitProcess.dwExitCode);
        m_client->processStopped(child);
        m_children.erase(child->id());
        if(m_pi.dwProcessId == child->id())
        {
            m_exitCode = debugEvent.u.ExitProcess.dwExitCode;
            for(auto it : m_children)//first stop everything and then cleanup
            {
                it.second->stop();
            }
        }
        delete child;
    }

    void readOutput(Pipe &p){
        BOOL bSuccess = FALSE;
        DWORD dwRead;
        bSuccess = PeekNamedPipe(p.hRead, NULL, 0, NULL, &dwRead, NULL);
        if(bSuccess && dwRead>0)
        {
            if(ReadFile(p.hRead, m_charBuffer, dwRead ,NULL, &p.overlapped))
            {
                MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, m_charBuffer, -1, m_wcharBuffer, dwRead+1);
                std::wstringstream ws;
                ws<<m_wcharBuffer<<std::endl;//TODO:detect if I get wchar_t or char from the subprocess

                if(p == m_stdout)
                    m_client->writeStdout(ws.str());
                else
                    m_client->writeErr(ws.str());

            }
        }
    }

    int run(){

        unsigned long debugConfig = DEBUG_ONLY_THIS_PROCESS;
        if(m_debugSubProcess)
            debugConfig = DEBUG_PROCESS;
        if(!CreateProcess ( (wchar_t*)m_program.c_str(), (wchar_t*)m_arguments.c_str(), NULL, NULL, TRUE,debugConfig, NULL,NULL,&m_si, &m_pi )){
            std::wstringstream ws;
            ws<<"Failed to start "<<m_program<<" "<<m_arguments<<" "<<GetLastError()<<std::endl;
            m_client->writeErr(ws.str());
            return -1;
        }

        DEBUG_EVENT debug_event = {0};


        do{
            readOutput(m_stdout);
            readOutput(m_stderr);
            if (WaitForDebugEvent(&debug_event,500)){
                readOutput(m_stdout);
                readOutput(m_stderr);
                switch(debug_event.dwDebugEventCode){
                case  OUTPUT_DEBUG_STRING_EVENT:
                    readDebugMSG(debug_event);
                    break;
                case CREATE_PROCESS_DEBUG_EVENT:
                    readProcessCreated(debug_event);
                    break;
                case EXIT_PROCESS_DEBUG_EVENT:
                    readProcessExited(debug_event);
                    break;
                default:
                    break;
                }
            }
            ContinueDebugEvent(debug_event.dwProcessId, debug_event.dwThreadId, DBG_CONTINUE);
        }while(m_children.size()>0);

        readOutput(m_stdout);
        readOutput(m_stderr);

        CloseHandle( m_pi.hProcess );
        CloseHandle( m_pi.hThread );
        return m_exitCode;
    }

    static BOOL CALLBACK shutdown(HWND hwnd, LPARAM procId)
    {
        DWORD currentProcId = 0;
        GetWindowThreadProcessId(hwnd, &currentProcId);
        if (currentProcId == (DWORD)procId)
        {
            if(SUCCEEDED(PostMessage(hwnd, WM_CLOSE , 0, 0)))
            {
                PostMessage(hwnd, WM_QUIT , 0, 0);
                SetEvent(SHUTDOWN_EVENT);
            }
        }
        return TRUE;
    }

    void stop()
    {

        EnumWindows(shutdown, m_pi.dwProcessId );
        if(WaitForSingleObject(SHUTDOWN_EVENT,50) != WAIT_OBJECT_0)
        {
            m_client->writeErr(L"Failed to post WM_CLOSE message");
        }
        if(FAILED(PostThreadMessage(m_pi.dwThreadId, WM_CLOSE , 0, 0)) || FAILED(PostThreadMessage(m_pi.dwThreadId, WM_QUIT , 0, 0)))
        {
            m_client->writeErr(L"Failed to post thred message");
        }

        if(WaitForSingleObject(m_pi.hProcess,10000) == WAIT_TIMEOUT)
        {
            m_children[m_pi.dwProcessId]->stop();
        }
    }



    VSDClient *m_client;
    std::wstring m_program;
    std::wstring m_arguments;
    bool m_debugSubProcess;

    //not thrad safe
    char m_charBuffer[VSD_BUFLEN];
    wchar_t m_wcharBuffer[VSD_BUFLEN];
    wchar_t m_wcharBuffer2[VSD_BUFLEN];
    unsigned long m_exitCode;

    STARTUPINFO m_si;
    PROCESS_INFORMATION m_pi;
    Pipe m_stdout;
    Pipe m_stderr;

    std::map <unsigned long, VSDChildProcess*> m_children;
};

VSDClient::VSDClient()
{

}

VSDClient::~VSDClient()
{

}

VSDProcess::VSDProcess(const std::wstring &program, const std::wstring &arguments, VSDClient *client)
    :d(new PrivateVSDProcess(program,arguments,client))
{

}

VSDProcess::~VSDProcess()
{
    delete d;
}

int VSDProcess::run()
{
    return d->run();
}

void VSDProcess::stop()
{
    d->stop();
}


void VSDProcess::debugSubProcess(bool b)
{
    d->m_debugSubProcess = b;
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

