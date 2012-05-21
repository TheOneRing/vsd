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

#include <windows.h>
#include <iostream>
#include <fstream>
#include <tchar.h>

#include "vsdprocess.h"

#ifndef _MSC_VER
#include <ext/stdio_filebuf.h>
#endif

using namespace libvsd;



void printHelp(){
	std::wcout<<L"Usage: vsd TARGET_APPLICATION [ARGUMENTS] [OPTIONS]"<<
	std::endl<<L"Options:"<<
	std::endl<<L"--vsdlog logFile\t File to log VSD output to"<<
	std::endl<<L"--help \t\t\t print this help";
	exit(0);
}

class VSDImp: public VSDClient{
public:
    VSDImp(wchar_t *out, wchar_t *in[],int len)
		:m_log(INVALID_HANDLE_VALUE)
	{
		wcscpy(out,L"\0");
        for(int i=1;i<len;++i){
			if(wcscmp(in[i],L"--vsdlog")==0){
				if(i+1<=len){
					i++;
					m_log = CreateFile(in[i++],                // name of the write
                       GENERIC_WRITE,          // open for writing
                       0,                      // do not share
                       NULL,                   // default security
                       CREATE_ALWAYS,             // create new file only
                       FILE_ATTRIBUTE_NORMAL,  // normal file
                       NULL); 
				}else
				{
					printHelp();
				}
			}else  if(wcscmp(in[i],L"--help")==0){
				printHelp();
            }else if(i>1){
				wcscat(out,L" \"");
				wcscat(out,in[i]);
				wcscat(out,L"\"");
			}
		}
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
		//else
		//{
		//	std::wcout<<L"LOG is null"<<std::endl;
		//}
	}

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
	wchar_t  *program = argv[1];
	wchar_t  *arguments = new wchar_t[MAX_PATH *2];
	VSDImp vsdimp(arguments,argv,argc);

	VSDProcess p(program,arguments,&vsdimp);
	delete [] arguments;

	unsigned long exitCode = p.run();
	std::wcout<<p.program()<<p.arguments()<<L" Exited with status: "<<exitCode<<std::endl;

	return exitCode;
}

