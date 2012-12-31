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
#include <signal.h>

#include "libvsd/vsdprocess.h"
#include "libvsd/vsdchildprocess.h"

#include <mutex>

using namespace libvsd;

#define VSDBUFF_SIZE 4096

void printHelp(){
    std::wcout<<L"Usage: vsd TARGET_APPLICATION [ARGUMENTS] [OPTIONS]"<<
                std::endl<<L"Options:"<<
                std::endl<<L"--vsd-log logFile\t File to log VSD output to"<<
                std::endl<<L"--vsd-all\t\t Debug also all processes created by TARGET_APPLICATION"<<
                std::endl<<L"--vsd-nc \t\t Monochrome output"<<
                std::endl<<L"--help \t\t\t print this help";
    exit(0);
}

class VSDImp: public VSDClient{
public:
    VSDImp(wchar_t *in[],int len)
        :m_exitCode(0)
        ,m_log(INVALID_HANDLE_VALUE)
        ,m_colored(true)
        ,m_html(true)
    {
        wchar_t  *program = in[1];
        wchar_t  *arguments = new wchar_t[MAX_PATH];
        bool withSubProcess = false;
        swprintf_s(arguments,MAX_PATH,L" ");

        for(int i=1;i<len;++i)
        {
            if(wcscmp(in[i],L"--vsd-log")==0)
            {
                if(i+1<=len)
                {
                    i++;
                    m_log = CreateFile(in[i], GENERIC_WRITE, FILE_SHARE_READ, NULL,CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                }
                else
                {
                    printHelp();
                }
            }
            else  if(wcscmp(in[i],L"--vsd-all")==0)
            {
                withSubProcess = true;
            }
            else  if(wcscmp(in[i],L"--vsd-nc")==0)
            {
                m_colored = false;
            }
            else  if(wcscmp(in[i],L"--help")==0)
            {
                printHelp();
            }else if(i>1)
            {
                swprintf_s(arguments,MAX_PATH,L"%ws \"%ws\" ",arguments,in[i]);
            }
        }

        m_hout = GetStdHandle( STD_OUTPUT_HANDLE  );
        GetConsoleScreenBufferInfo( m_hout, &m_consoleSettings);

        htmlHEADER(program, arguments);
        swprintf_s(m_wcharBuffer,L"%ws %ws\n", program, arguments);
        print(m_wcharBuffer, FOREGROUND_BLUE | FOREGROUND_INTENSITY);

        m_process = new VSDProcess(program, arguments, this);
        m_process->debugSubProcess(withSubProcess);
        delete [] arguments;
    }

    ~VSDImp()
    {

        delete m_process;
        if(m_log != INVALID_HANDLE_VALUE){
            htmlFOOTER();
            CloseHandle(m_log);
        }
        SetConsoleTextAttribute( m_hout, m_consoleSettings.wAttributes );
        CloseHandle(m_hout);
    }

    void run()
    {
        m_exitCode = m_process->run();

    }

    void htmlHEADER(const wchar_t *program,const wchar_t *arguments)
    {
        if(!m_html)
            return;
        printFilePlain("<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"UTF-8\" />\n<title>");
        swprintf_s(m_wcharBuffer,L"%ws %ws", program, arguments);
        printFile(m_wcharBuffer,0);
        printFilePlain("</title>\n</head>\n\n<body>");

    }

    void htmlFOOTER()
    {
        if(!m_html)
            return;
        printFilePlain("</body>\n\n</html>\n");
    }

    void printFilePlain(const char* data)
    {
        DWORD dwRead;
        WriteFile(m_log,data, strlen(data) * sizeof(char), &dwRead,NULL);
    }

    void printFile(const wchar_t* data,WORD color)
    {
        if(m_log == INVALID_HANDLE_VALUE)
            return;
        char tag[20];
        DWORD dwRead;
        if(m_html && color != 0)
        {
            switch(color)
            {
            case FOREGROUND_BLUE:
            case FOREGROUND_BLUE | FOREGROUND_INTENSITY:
                strcpy(tag, "blue\0");
                break;
            case FOREGROUND_GREEN:
            case FOREGROUND_GREEN | FOREGROUND_INTENSITY:
                strcpy(tag, "green\0");
                break;
            case FOREGROUND_RED:
            case FOREGROUND_RED | FOREGROUND_INTENSITY:
                strcpy(tag, "red\0");
                break;
            default:
                strcpy(tag, "black\0");
            }
            printFilePlain("<p style=\"color:\0");
            printFilePlain(tag);
            printFilePlain("\">\0");
            swprintf_s(m_wcharBuffer2,L"%ws", data);
            WriteFile(m_log,m_wcharBuffer2, wcslen(m_wcharBuffer2) * sizeof(wchar_t), &dwRead,NULL);
            printFilePlain("</p>\n");
        }
        else
        {
            WriteFile(m_log,data, wcslen(data) * sizeof(wchar_t), &dwRead,NULL);
        }
    }

    void print(const wchar_t* data,WORD color)
    {        
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock(mutex);
        if( m_colored )
            SetConsoleTextAttribute( m_hout, color);
        std::wcout<<data;
        std::wcout.flush();
        printFile(data,color);
    }

    void writeStdout(const wchar_t *data)
    {
        print(data, m_consoleSettings.wAttributes);
    }

    void writeErr(const wchar_t *data)
    {
        print(data, FOREGROUND_RED);
    }

    void writeDebug(const VSDChildProcess *process,const wchar_t *data)
    {
        swprintf_s(m_wcharBuffer, L"%ws: %ws", process->name(), data);
        print(m_wcharBuffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }
    void processStarted(const VSDChildProcess *process)
    {
        swprintf_s(m_wcharBuffer,L"Process Created: %ws \n", process->path());
        print(m_wcharBuffer, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    }

    void processStopped(const VSDChildProcess *process)
    {
        swprintf_s(m_wcharBuffer, L"Process Stopped: %ws  With exit Code: %i  After: %i:%i:%lld:%lld \n", process->path(), process->exitCode(),
                   std::chrono::duration_cast<std::chrono::hours>(process->time()),
                   std::chrono::duration_cast<std::chrono::minutes>(process->time()),
                   std::chrono::duration_cast<std::chrono::seconds>(process->time()),
                   std::chrono::duration_cast<std::chrono::milliseconds>(process->time()));
        print(m_wcharBuffer,  FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    }


    void stop()
    {
        m_process->stop();
    }

    int m_exitCode;
private:
    VSDProcess *m_process;
    HANDLE m_log;
    wchar_t m_wcharBuffer[VSDBUFF_SIZE ];
    wchar_t m_wcharBuffer2[VSDBUFF_SIZE ];
    HANDLE m_hout;
    CONSOLE_SCREEN_BUFFER_INFO m_consoleSettings;
    bool m_colored;
    bool m_html;


};


static VSDImp *vsdimp = NULL;

void sighandler(int sig)
{

    if(vsdimp != NULL)
    {
        vsdimp->stop();
    }
}


int main()
{
    int argc;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if(argc<2){
        printHelp();
    }

    vsdimp = new VSDImp(argv,argc);

    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);

    vsdimp->run();

    int ret = vsdimp->m_exitCode;
    delete vsdimp;
    vsdimp = NULL;
    return ret;
}

