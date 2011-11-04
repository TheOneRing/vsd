#include "process.h"

#include <windows.h>
#include <WinBase.h>
#include <atlstr.h>
#include <iostream>

#include <stdlib.h>
#include "atlbase.h"
#include "atlstr.h"
#include "comutil.h"

using namespace libvsd;

Process::Process(wchar_t *program,wchar_t * arguments):
m_readyAnsi(NULL),
    m_readyUTF8(NULL)
{
    m_program = new wchar_t[wcslen(program)];
    wcscpy(m_program,program);
    m_arguments = new wchar_t[wcslen(arguments)];
    wcscpy(m_arguments,arguments);


    ZeroMemory( &m_si, sizeof(m_si) ); 
    m_si.cb = sizeof(m_si); 
    ZeroMemory( &m_pi, sizeof(m_pi) );

    
    std::wcout<<m_program<<m_arguments<<std::endl;

}

Process::~Process(){
    //delete [] m_program;
    //delete [] m_arguments;
}

int Process::run(){

    if(!CreateProcess ( m_program, m_arguments, NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS, NULL,NULL,&m_si, &m_pi )){
        return -1;
    }
    DEBUG_EVENT debug_event = {0};
    unsigned long exitCode = 0;
    bool run = true;
    while(run)
    {
        if (!WaitForDebugEvent(&debug_event, INFINITE))
            return -1;
        switch(debug_event.dwDebugEventCode){
        case  OUTPUT_DEBUG_STRING_EVENT:
            readDebugMSG(debug_event);
            break;
        case EXIT_PROCESS_DEBUG_EVENT:
            exitCode = debug_event.u.ExitProcess.dwExitCode;
            run = false;
            break;
        default:
            break;
        }


        ContinueDebugEvent(debug_event.dwProcessId,debug_event.dwThreadId,DBG_CONTINUE);
    }

    CloseHandle( m_pi.hProcess );
    CloseHandle( m_pi.hThread );
    return exitCode;
}

void Process::readDebugMSG(DEBUG_EVENT &debugEvent){
    OUTPUT_DEBUG_STRING_INFO  &DebugString = debugEvent.u.DebugString;
    wchar_t *msg = new WCHAR[DebugString.nDebugStringLength];

    ReadProcessMemory(m_pi.hProcess,DebugString.lpDebugStringData,msg,DebugString.nDebugStringLength, NULL);

    if ( DebugString.fUnicode ){	
        if(m_readyUTF8)
            m_readyUTF8(msg);
    }else{
        if(m_readyAnsi)
            m_readyAnsi((char*)msg);
    }


    delete []msg;

}

wchar_t *Process::program(){
    return m_program;
}

wchar_t *Process::arguments(){
    return m_arguments;
}

void Process::setANSICallback(readyReadANSI *ansi){
    m_readyAnsi = ansi;
}

void Process::setUTF8Callback(readyReadUTF8 *utf8){
    m_readyUTF8 = utf8;
}


