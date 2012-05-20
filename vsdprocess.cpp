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
** License along with FFmpeg; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/


#include "vsdprocess.h"

#include <windows.h>
#include <winbase.h>
#include <iostream>

#include <stdlib.h>

#ifdef __MINGW64_VERSION_MAJOR
#define PIPE_REJECT_REMOTE_CLIENTS 0x00000008
#endif
//inspired by https://qt.gitorious.org/qt-labs/jom/blobs/master/src/jomlib/process.cpp
using namespace libvsd;

class VSDProcess::PrivateVSDProcess{
public:
	struct Pipe
	{
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

		HANDLE hWrite;
		HANDLE hRead;
		OVERLAPPED overlapped;
	};

	PrivateVSDProcess(const wchar_t *program,const wchar_t * arguments,VSDClient *client)
		:m_client(client)
		,m_program(SysAllocString(program))
		,m_arguments(SysAllocString(arguments))
	{

		SECURITY_ATTRIBUTES sa = {0};
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = TRUE;


		if (!setupPipe(m_stdout, &sa)){
			std::wcerr<<L"Cannot setup pipe for sout."<<std::endl;
			exit(1);
		}

		ZeroMemory( &m_si, sizeof(m_si) );
		m_si.cb = sizeof(m_si); 
		m_si.dwFlags |= STARTF_USESTDHANDLES;
		m_si.hStdOutput = m_stdout.hWrite;
		m_si.hStdError =  m_stdout.hWrite;

		ZeroMemory( &m_pi, sizeof(m_pi) );

	}

	~PrivateVSDProcess()
	{
		SysFreeString(m_program);
		SysFreeString(m_arguments);
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
			std::wcerr<<L"Creation of the pipe failed "<<GetLastError()<<std::endl;
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
			std::wcerr<<L"Creation of the pipe failed "<<GetLastError()<<std::endl;
			return false;
		}
		ConnectNamedPipe(pipe.hRead, NULL);
		return true;
	}

	void readDebugMSG(DEBUG_EVENT &debugEvent){
		OUTPUT_DEBUG_STRING_INFO  &DebugString = debugEvent.u.DebugString;

		wchar_t *msg = new wchar_t[DebugString.nDebugStringLength];
		ReadProcessMemory(m_pi.hProcess,DebugString.lpDebugStringData,msg,DebugString.nDebugStringLength, NULL);


		if ( DebugString.fUnicode ){
			m_client->write(msg);
		}else{
			wchar_t *message = new wchar_t[DebugString.nDebugStringLength];
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char*)msg, -1, message, DebugString.nDebugStringLength+1);
			m_client->write(message);
			delete [] message;
		}
		delete [] msg;

	}

	int run(){

		if(!CreateProcess ( m_program, m_arguments, NULL, NULL, TRUE, DEBUG_ONLY_THIS_PROCESS, NULL,NULL,&m_si, &m_pi )){
			return -1;
		}

		DEBUG_EVENT debug_event = {0};
		unsigned long exitCode = 0;
		bool run = true;
		const size_t buflen = 4096;
		char chBuf[buflen];
		wchar_t chwBuf[buflen];
		BOOL bSuccess = FALSE;
		DWORD dwRead;


		while(run)
		{
			bSuccess = PeekNamedPipe(m_stdout.hRead, NULL, 0, NULL, &dwRead, NULL);
			if(bSuccess && dwRead>0){//TODO:detect if I get wchar_t or char from the subprocess
				if(ReadFile(m_stdout.hRead, chBuf,dwRead,NULL,&m_stdout.overlapped)){
					MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, chBuf, -1, chwBuf, dwRead+1);
					wcscpy(chwBuf+dwRead,L"\0");
					m_client->write(chwBuf);
				}
			}
			if (WaitForDebugEvent(&debug_event,500)){
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
			}

			ContinueDebugEvent(debug_event.dwProcessId,debug_event.dwThreadId,DBG_CONTINUE);
		}


		CloseHandle( m_pi.hProcess );
		CloseHandle( m_pi.hThread );
		return exitCode;
	}



	VSDClient *m_client;
	wchar_t *m_program;
	wchar_t *m_arguments;

	STARTUPINFO m_si; 
	PROCESS_INFORMATION m_pi; 
	Pipe m_stdout;
};

VSDClient::VSDClient()
{

}

VSDClient::~VSDClient()
{

}

VSDProcess::VSDProcess(const wchar_t *program,const wchar_t * arguments,VSDClient *client)
	:d(new PrivateVSDProcess(program,arguments,client))
{
	std::wcout<<d->m_program<<d->m_arguments<<std::endl;

}


VSDProcess::~VSDProcess()
{
	delete d;
}

int VSDProcess::run()
{
	return d->run();
}



const wchar_t *VSDProcess::program() const
{
	return d->m_program;
}

const wchar_t *VSDProcess::arguments() const
{
	return d->m_arguments;
}




