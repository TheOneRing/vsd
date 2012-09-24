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
{
    wchar_t buff[VSD_BUFLEN];
    GetFinalPathNameByHandle(fileHandle,buff,VSD_BUFLEN,FILE_NAME_OPENED);
    m_path = SysAllocString(buff+4);
    m_name = SysAllocString(m_path+findLastBackslash(m_path));

}

VSDChildProcess::~VSDChildProcess()
{
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
    return m_name;
}

const wchar_t *VSDChildProcess::name() const
{
    return m_name;
}

const double VSDChildProcess::time() const
{
    return ::difftime(::time(NULL),m_startTime);
}

const unsigned long VSDChildProcess::id() const
{
    return m_id;
}

long VSDChildProcess::findLastBackslash(const wchar_t *in)
{
    for(int i=wcslen(in);i>0;--i){
        if(in[i] == '\\')
            return i+1;
    }
}
