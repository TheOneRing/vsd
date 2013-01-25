/*
    VSD prints debugging messages of applications and their
    sub-processes to console and supports logging of their output.
    Copyright (C) 2012  Patrick von Reth <vonreth@kde.org>


    VSD is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VSD is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with SnarlNetworkBridge.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "libvsd/vsdprocess.h"
#include "libvsd/vsdchildprocess.h"

#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <signal.h>
#include <mutex>

using namespace libvsd;

void printHelp(){
    std::wcout<<L"Usage: vsd TARGET_APPLICATION [ARGUMENTS] [OPTIONS]"<<std::endl<<
                L"Options:"<<std::endl<<
                L"--vsd-log logFile\t Write a log in colored html to logFile"<<std::endl<<
                L"--vsd-logplain logFile\t Write a log to logFile"<<std::endl<<
                L"--vsd-all\t\t Debug also all processes created by TARGET_APPLICATION"<<std::endl<<
                L"--vsd-nc \t\t Monochrome output"<<std::endl<<
                L"--help \t\t\t print this help"<<std::endl<<
                L"--version\t\t print version and copyright information"<<std::endl;
    exit(0);
}

void printVersion(){
    std::wcout<<L"VSD version 0.5"<<std::endl<<
                std::endl<<
                L"Copyright (C) 2012  Patrick von Reth <vonreth@kde.org>"<<std::endl<<
                std::endl<<
                L"VSD is free software: you can redistribute it and/or modify"<<std::endl<<
                L"it under the terms of the GNU Lesser General Public License as published by"<<std::endl<<
                L"the Free Software Foundation, either version 3 of the License, or"<<std::endl<<
                L"(at your option) any later version."<<std::endl;
    exit(0);
}

class VSDImp: public VSDClient{
public:
    VSDImp(wchar_t *in[],int len) :
        m_exitCode(0),
        m_log(INVALID_HANDLE_VALUE),
        m_colored(true),
        m_html(true)
    {
        std::wstring program(in[1]);
        std::wstringstream arguments;
        bool withSubProcess = false;
        for(int i=1;i<len;++i)
        {
            std::wstring arg(in[i]);
            if(arg == L"--vsd-log")
            {
                i = initLog(in,i,len);
            }
            else if(arg == L"--vsd-logplain")
            {
                m_html = false;
                i = initLog(in,i,len);
            }
            else  if(arg == L"--vsd-all")
            {
                withSubProcess = true;
            }
            else  if(arg == L"--vsd-nc")
            {
                m_colored = false;
            }
            else  if(arg == L"--help")
            {
                printHelp();
            }
            else  if(arg == L"--version")
            {
                printVersion();
            }
            else if(i>1)
            {
                arguments<<"\""<<arg<<"\" ";
            }
        }

        m_hout = GetStdHandle( STD_OUTPUT_HANDLE  );
        GetConsoleScreenBufferInfo( m_hout, &m_consoleSettings);

        htmlHEADER(program, arguments.str());
        std::wstringstream ws;
        ws<<program<<" "<<arguments.str()<<std::endl;
        print(ws.str(), FOREGROUND_BLUE | FOREGROUND_INTENSITY);

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

    int initLog(wchar_t *in[],int pos,int len)
    {
        if(pos+1<=len)
        {
            m_log = CreateFile(in[++pos], GENERIC_WRITE, FILE_SHARE_READ, NULL,CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        }
        else
        {
            printHelp();
        }
        return pos;
    }

    void run()
    {
        m_exitCode = m_process->run();
    }

    void htmlHEADER(const std::wstring &program,const std::wstring &arguments)
    {
        if(!m_html)
            return;
        std::stringstream ss;
        ss<<"<!DOCTYPE html>\n"<<
            "<html>\n"<<
            "<head>\n"<<
            "<meta charset=\"UTF-8\" />\n"<<
            "<title>VSD "<<
            std::string(program.begin(),program.end())<<" "<<
            std::string(arguments.begin(),arguments.end())<<
            "</title>\n"<<
            "</head>\n"<<
            "<body>";
        printFilePlain(ss.str());

    }

    void htmlFOOTER()
    {
        if(!m_html)
            return;
        printFilePlain("</body>\n\n</html>\n");
    }

    void printFilePlain(const std::string &data)
    {
        DWORD dwRead;
        WriteFile(m_log,data.c_str(), data.length() * sizeof(char), &dwRead,NULL);
    }

    void printFile(const std::wstring &data,WORD color)
    {
        if(m_log == INVALID_HANDLE_VALUE)
            return;
        std::stringstream ss;
        DWORD dwRead;
        if(m_html && color != 0)
        {
            ss<<"<p style=\"color:";
            switch(color)
            {
            case FOREGROUND_BLUE:
            case FOREGROUND_BLUE | FOREGROUND_INTENSITY:
                ss<<"blue";
                break;
            case FOREGROUND_GREEN:
            case FOREGROUND_GREEN | FOREGROUND_INTENSITY:
                ss<<"green";
                break;
            case FOREGROUND_RED:
            case FOREGROUND_RED | FOREGROUND_INTENSITY:
                ss<<"red";
                break;
            default:
                ss<<"black";
            }
            ss<<"\">";
            printFilePlain(ss.str());
            WriteFile(m_log,data.c_str(), data.size()* sizeof(wchar_t), &dwRead,NULL);
            printFilePlain("</p>\n");
        }
        else
        {
            WriteFile(m_log,data.c_str(), data.size()* sizeof(wchar_t), &dwRead,NULL);
        }
    }

    void print(const std::wstring &data,WORD color)
    {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock(mutex);
        if( m_colored )
            SetConsoleTextAttribute( m_hout, color);
        std::wcout<<data;
        printFile(data,color);
    }

    void writeStdout(const std::wstring &data)
    {
        print(data, m_consoleSettings.wAttributes);
    }

    void writeErr(const std::wstring &data)
    {
        print(data, FOREGROUND_RED | FOREGROUND_INTENSITY);
    }

    void writeDebug(const VSDChildProcess *process,const std::wstring &data)
    {
        std::wstringstream ws;
        ws<<process->name()<<": "<<data;
        print(ws.str(), FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }
    void processStarted(const VSDChildProcess *process)
    {
        std::wstringstream ws;
        ws<<"Process Created: "<<process->path()<<std::endl;
        print(ws.str(), FOREGROUND_BLUE | FOREGROUND_INTENSITY);
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
        print(ws.str(),  FOREGROUND_BLUE | FOREGROUND_INTENSITY);
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

