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

#include <windows.h>
#include <iostream>
#include <fstream>
#include <tchar.h>

#include "libvsd/vsdprocess.h"
#include "libvsd/vsdchildprocess.h"

using namespace libvsd;



void printHelp(){
    std::wcout<<L"Usage: vsd TARGET_APPLICATION [ARGUMENTS] [OPTIONS]"<<
                std::endl<<L"Options:"<<
                std::endl<<L"--vsdlog logFile\t File to log VSD output to"<<
                std::endl<<L"--vsdall\t\t Debug also all processes created by TARGET_APPLICATION"<<
                std::endl<<L"--help \t\t\t print this help";
    exit(0);
}

class VSDImp: public VSDClient{
public:
    VSDImp(wchar_t *in[],int len)
        :m_exitCode(0)
        ,m_log(INVALID_HANDLE_VALUE)
    {
        wchar_t  *program = in[1];
        wchar_t  *arguments = new wchar_t[MAX_PATH];
        bool withSubProcess = false;
        wcscpy(arguments,L"\0");
        for(int i=1;i<len;++i){
            if(wcscmp(in[i],L"--vsdlog")==0){
                if(i+1<=len){
                    i++;
                    m_log = CreateFile(in[i++],GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
                }else{
                    printHelp();
                }
            }else  if(wcscmp(in[i],L"--vsdall")==0){
                withSubProcess = true;

            }else  if(wcscmp(in[i],L"--help")==0){
                printHelp();
            }else if(i>1){
                wcscat(arguments,L" \"");
                wcscat(arguments,in[i]);
                wcscat(arguments,L"\"");
            }
        }

        VSDProcess p(program,arguments,this);
        delete [] arguments;
        p.debugSubProcess(withSubProcess);
        m_exitCode = p.run();
        std::wcout<<p.program()<<p.arguments()<<L" Exited with status: "<<m_exitCode<<std::endl;
    }


    ~VSDImp()
    {
        if(m_log != INVALID_HANDLE_VALUE){
            CloseHandle(m_log);
        }
    }

    void write(const wchar_t *data)
    {
        std::wcout<<data;
        if(m_log != INVALID_HANDLE_VALUE){
            size_t len = wcslen(data);
            DWORD dwRead;
            WriteFile(m_log,data,wcslen(data)*sizeof(wchar_t),&dwRead,NULL);
        }
    }

    void processStarted(const VSDChildProcess *process)
    {
        std::wcout<<L"Process Created: "<<process->path()<<std::endl;
    }

    void processStopped(const VSDChildProcess *process)
    {
        std::wcout<<L"Process Stopped: "<<process->path()<<" With exit Code: "<<process->exitCode()<<" After: "<<process->time()<<" seconds"<<std::endl;;
    }



    int m_exitCode;
private:
    HANDLE m_log;

};





int main()
{
    int argc;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if(argc<2){
        printHelp();
    }


    VSDImp vsdimp(argv,argc);
    return vsdimp.m_exitCode;
}

