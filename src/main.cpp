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
#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
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
        std::wstringstream arguments;
        bool withSubProcess = false;
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
                arguments<<"\""<<in[i]<<"\" ";
            }
        }

        m_hout = GetStdHandle( STD_OUTPUT_HANDLE  );
        GetConsoleScreenBufferInfo( m_hout, &m_consoleSettings);

        htmlHEADER(program, arguments);
        std::wstringstream ws;
        ws<<program<<arguments.str()<<std::endl;
        print(ws, FOREGROUND_BLUE | FOREGROUND_INTENSITY);

        m_process = new VSDProcess(program, arguments.str(), this);
        m_process->debugSubProcess(withSubProcess);
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

    void htmlHEADER(const wchar_t *program,const std::wstringstream &arguments)
    {
        if(!m_html)
            return;
        printFilePlain("<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"UTF-8\" />\n<title>");
        std::wstringstream ws;
        ws<<program<<" "<<arguments;
        printFile(ws,0);
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

    void printFile(const std::wstringstream &data,WORD color)
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
            WriteFile(m_log,data.str().c_str(), data.str().size()* sizeof(wchar_t), &dwRead,NULL);
            printFilePlain("</p>\n");
        }
        else
        {
            WriteFile(m_log,data.str().c_str(), data.str().size()* sizeof(wchar_t), &dwRead,NULL);
        }
    }

    void print(const std::wstringstream &data,WORD color)
    {        
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock(mutex);
        if( m_colored )
            SetConsoleTextAttribute( m_hout, color);
        std::wcout<<data.str();
        printFile(data,color);
    }

    void writeStdout(const std::wstringstream &data)
    {
        print(data, m_consoleSettings.wAttributes);
    }

    void writeErr(const std::wstringstream &data)
    {
        print(data, FOREGROUND_RED);
    }

    void writeDebug(const VSDChildProcess *process,const std::wstringstream &data)
    {
        std::wstringstream ws;
        ws<<process->name()<<": "<<data.str();
        print(ws, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }
    void processStarted(const VSDChildProcess *process)
    {
        std::wstringstream ws;
        ws<<"Process Created: "<<process->path()<<std::endl;
        print(ws, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    }

    void processStopped(const VSDChildProcess *process)
    {
        std::wstringstream ws;
        ws<<"Process Stopped: "
        <<process->path()
        <<"  With exit Code: "
        << process->exitCode()
        <<"  After: "
        <<std::chrono::duration_cast<std::chrono::hours>(process->time()).count()<<":"
        <<std::chrono::duration_cast<std::chrono::minutes>(process->time()).count()<<":"
        <<std::chrono::duration_cast<std::chrono::seconds>(process->time()).count()<<":"
        <<std::chrono::duration_cast<std::chrono::milliseconds>(process->time()).count()
        <<std::endl;
        print(ws,  FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    }


    void stop()
    {
        m_process->stop();
    }

    int m_exitCode;
private:
    VSDProcess *m_process;
    HANDLE m_log;
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

