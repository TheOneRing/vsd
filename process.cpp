#include "process.h"

#include <windows.h>
#include <winbase.h>
#include <iostream>

#include <stdlib.h>

using namespace libvsd;

struct Pipe
{
	Pipe()
		: hWrite(INVALID_HANDLE_VALUE)
		, hRead(INVALID_HANDLE_VALUE)
	{}

	~Pipe()
	{
		if (hWrite != INVALID_HANDLE_VALUE)
			CloseHandle(hWrite);
		if (hRead != INVALID_HANDLE_VALUE)
			CloseHandle(hRead);
	}

	HANDLE hWrite;
	HANDLE hRead;
};

static bool setupPipe(Pipe &pipe, SECURITY_ATTRIBUTES *sa)
{
	if (!CreatePipe(&pipe.hRead, &pipe.hWrite, sa, 0)) {
		std::wcerr<<L"CreatePipe failed with error code "<<GetLastError()<<std::endl;
		return false;
	}
	if (!SetHandleInformation(pipe.hRead, HANDLE_FLAG_INHERIT, 0)) {
		std::wcerr<<L"SetHandleInformation failed with error code "<<GetLastError()<<std::endl;
		return false;
	}
	return true;
}

Process::Process(wchar_t *program,wchar_t * arguments):
	m_readyUTF8(NULL),
	m_arguments(NULL)
{
	m_program = SysAllocString(program);


	m_arguments = SysAllocString(arguments);


	std::wcout<<m_program<<m_arguments<<std::endl;

	SECURITY_ATTRIBUTES sa = {0};
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	Pipe sout;
	Pipe serr;

	if (!setupPipe(sout, &sa)){
		std::wcerr<<L"Cannot setup pipe for sout."<<std::endl;
		exit(1);
	}


	DuplicateHandle(GetCurrentProcess(),sout.hWrite, GetCurrentProcess(),
		&serr.hWrite, 0, TRUE, DUPLICATE_SAME_ACCESS);

	m_stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	ZeroMemory( &m_si, sizeof(m_si) );
	m_si.cb = sizeof(m_si); 
	m_si.dwFlags |= STARTF_USESTDHANDLES;
	m_si.hStdOutput = sout.hWrite;
	m_si.hStdError = serr.hWrite;

	ZeroMemory( &m_pi, sizeof(m_pi) );
}


Process::~Process(){
	SysFreeString(m_program);
	SysFreeString(m_arguments);
}

int Process::run(){

	if(!CreateProcess ( m_program, m_arguments, NULL, NULL, TRUE, DEBUG_ONLY_THIS_PROCESS, NULL,NULL,&m_si, &m_pi )){
		return -1;
	}

	DEBUG_EVENT debug_event = {0};
	unsigned long exitCode = 0;
	bool run = true;
	DWORD dwRead, dwWritten;
	const size_t buflen = 4096;
	char chBuf[buflen];
	BOOL bSuccess = FALSE;

	while(run)
	{
		bSuccess = ReadFile(m_si.hStdError, chBuf, buflen, &dwRead, NULL);
		if(bSuccess || !dwRead == 0)
			WriteFile(m_stdOut, chBuf, dwRead, &dwWritten, NULL);
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
/**
Client has to Free  the string
**/

void Process::readDebugMSG(DEBUG_EVENT &debugEvent){
	OUTPUT_DEBUG_STRING_INFO  &DebugString = debugEvent.u.DebugString;
	wchar_t *message;
	
	wchar_t *msg = new wchar_t[DebugString.nDebugStringLength];
	ReadProcessMemory(m_pi.hProcess,DebugString.lpDebugStringData,msg,DebugString.nDebugStringLength, NULL);


	if ( DebugString.fUnicode ){
			message = SysAllocString(msg);
	}else{
			OLECHAR *olechar = new OLECHAR[DebugString.nDebugStringLength];
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char*)msg, -1, olechar, DebugString.nDebugStringLength+1);  
			message = SysAllocString(olechar);
			delete []olechar;
	}
	delete []msg;

		
	m_readyUTF8(message);
}

wchar_t *Process::program(){
	return m_program;
}

wchar_t *Process::arguments(){
	return m_arguments;
}


void Process::setUTF8Callback(readyReadUTF8 *utf8){
	m_readyUTF8 = utf8;
}


