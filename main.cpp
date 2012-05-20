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



std::wostream *open_ofstream(wchar_t* name, const std::ios_base::openmode  mode)
{
#ifndef _MSC_VER
	std::wcout<<L"opening file"<<name<<std::endl;
	FILE* c_file = _wfopen( name, L"w,ccs=UNICODE" );
	__gnu_cxx::stdio_filebuf<wchar_t>* buffer = new __gnu_cxx::stdio_filebuf<wchar_t>( c_file, std::ios::out, 1 );

	return new std::wostream(buffer);
#else
	return new std::wofstream(name,mode);
#endif
}

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
		:m_log(NULL)
	{
		printHelp();
		wcscpy(out,L"\0");
		for(int i=2;i<len;++i){
			if(wcscmp(in[i],L"--vsdlog")==0){
				if(i+1<=len){
					i++;
					m_log = open_ofstream(in[i++], std::ios::app); 
				}else
				{
					printHelp();
				}
				continue;
			}
			if(wcscmp(in[i],L"--help")==0){
				printHelp();
			}
			
			wcscat(out,L" \"");
			wcscat(out,in[i]);
			wcscat(out,L"\"");
		}
	}


	~VSDImp()
	{
		if(m_log)
			m_log->flush();
	}

	void write(const wchar_t *data)
	{
		std::wcout<<data;
		if(m_log)
			*m_log<<data;
		//else
		//{
		//	std::wcout<<L"LOG is null"<<std::endl;
		//}
	}

private:
	std::wostream *m_log;
};





int _tmain(int argc,wchar_t *argv[])
{
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

