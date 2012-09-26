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
#include <stdlib.h>
#include <wchar.h>

#include "libvsd/vsdprocess.h"
#include "libvsd/vsdchildprocess.h"

using namespace libvsd;

#define VSDBUFF_SIZE 4096

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

        m_hout = GetStdHandle( STD_OUTPUT_HANDLE  );
        GetConsoleScreenBufferInfo( m_hout, &m_consoleSettings);

        SetConsoleTextAttribute( m_hout, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        std::wcout<<program<<arguments<<std::endl;

        VSDProcess p(program,arguments,this);
        delete [] arguments;
        p.debugSubProcess(withSubProcess);
        m_exitCode = p.run();

        SetConsoleTextAttribute( m_hout, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        std::wcout<<p.program()<<p.arguments()<<L" Exited with status: "<<m_exitCode<<std::endl;


    }


    ~VSDImp()
    {
        if(m_log != INVALID_HANDLE_VALUE){
            CloseHandle(m_log);
        }
        SetConsoleTextAttribute( m_hout, m_consoleSettings.wAttributes );
        CloseHandle(m_hout);
    }

    void print(const wchar_t* data)
    {
        std::wcout<<data;
        if(m_log != INVALID_HANDLE_VALUE){
            DWORD dwRead;
            WriteFile(m_log,data, wcslen(data) * sizeof(wchar_t), &dwRead,NULL);
        }
    }

    void writeStdout(const wchar_t *data)
    {
        SetConsoleTextAttribute( m_hout, m_consoleSettings.wAttributes );
        print(data);
    }

    void writeErr(const wchar_t *data)
    {
        SetConsoleTextAttribute( m_hout, FOREGROUND_RED);
        print(data);
    }

    void writeDebug(const VSDChildProcess *process,const wchar_t *data)
    {
        SetConsoleTextAttribute( m_hout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        swprintf(m_wcharBuffer, L"%ls: %ls", process->name(), data);
        print(m_wcharBuffer);
    }
    void processStarted(const VSDChildProcess *process)
    {
        SetConsoleTextAttribute( m_hout, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        swprintf(m_wcharBuffer,L"Process Created: %ls \n", process->path());
        print(m_wcharBuffer);
    }

    void processStopped(const VSDChildProcess *process)
    {
        SetConsoleTextAttribute( m_hout, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        swprintf(m_wcharBuffer, L"Process Stopped: %ls  With exit Code: %i  After: %d seconds\n", process->path(), process->exitCode(), process->time());
        print(m_wcharBuffer);
    }



    int m_exitCode;
private:
    HANDLE m_log;
    wchar_t m_wcharBuffer[VSDBUFF_SIZE ];
    HANDLE m_hout;
    CONSOLE_SCREEN_BUFFER_INFO m_consoleSettings;

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

