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

#include "vsdchildprocess.h"

#include <windows.h>
#include <winbase.h>
#include <time.h>

#include <iostream>

using namespace libvsd;

#define VSD_BUFLEN 4096


VSDChildProcess::VSDChildProcess(const unsigned long id, const HANDLE fileHandle)
    :m_id(id)
    ,m_handle(OpenProcess(PROCESS_ALL_ACCESS, FALSE, id))
    ,m_startTime(::time(NULL))
    ,m_duration(0)
    ,m_exitCode(STILL_ACTIVE)
    ,m_stopped(false)
{
    wchar_t buff[VSD_BUFLEN];
    GetFinalPathNameByHandle(fileHandle,buff,VSD_BUFLEN,FILE_NAME_OPENED);
    m_path = SysAllocString(buff+4);
    m_name = SysAllocString(m_path+findLastBackslash(m_path));

}

VSDChildProcess::~VSDChildProcess()
{

    if(!m_stopped)
    {
        std::wcerr<<"Killing "<<path()<<" subprocess"<<std::endl;
        TerminateProcess(handle(), 0);
        m_exitCode = -1;
    }

    SysFreeString(m_name);
    SysFreeString(m_path);
    CloseHandle(m_handle);
}

const HANDLE &VSDChildProcess::handle() const
{
    return m_handle;
}

const wchar_t *VSDChildProcess::path() const
{
    return m_path;
}

const wchar_t *VSDChildProcess::name() const
{
    return m_name;
}

const double VSDChildProcess::time() const
{
    if(m_stopped)
        return m_duration;
    return ::difftime(::time(NULL),m_startTime);
}

const unsigned long VSDChildProcess::id() const
{
    return m_id;
}

const int VSDChildProcess::exitCode() const
{
    return m_exitCode;
}

long VSDChildProcess::findLastBackslash(const wchar_t *in)
{
    for(int i=wcslen(in);i>0;--i){
        if(in[i] == '\\')
            return i+1;
    }
}


void VSDChildProcess::processStopped(const int exitCode)
{
    m_exitCode = exitCode;
    m_duration = ::difftime(::time(NULL),m_startTime);
    m_stopped = true;
}
