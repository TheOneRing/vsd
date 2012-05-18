
#include <windows.h>
#include <iostream>
#include <fstream>
#include <tchar.h>
#include "process.h"

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



static std::wostream *_log = NULL;

void writeUTF8(wchar_t *w){
	std::wcout<<w;
	if(_log)
		*_log<<w;
	//else
	//{
	//	std::wcout<<L"LOG is null"<<std::endl;
	//}
	SysFreeString(w);
}

void initArgs(wchar_t *out, wchar_t *in[],int len){
	wcscpy(out,L"\0");
	for(int i=2;i<len;++i){
		if(wcscmp(in[i],L"--log")==0){
			if(i+1<=len){
				i++;
				_log = open_ofstream(in[i++], std::ios::app); 
			}else
			{
				std::wcout<<"Error to few arguments"<<std::endl;
			}
			continue;
		}

		wcscat(out,L" \"");
		wcscat(out,in[i]);
		wcscat(out,L"\"");
	}
}

int _tmain(int argc,wchar_t *argv[])
{
	if(argc<2)
		return -1;
	wchar_t  *program = argv[1];
	wchar_t  *arguments = new wchar_t[MAX_PATH *2];
	initArgs(arguments,argv,argc);

	Process p(program,arguments);
	delete [] arguments;


	p.setUTF8Callback(writeUTF8);




	unsigned long exitCode = p.run();
	char buf[20];
	ltoa(exitCode,buf,10);
	std::wcout<<p.program()<<p.arguments()<<L" Exited with status: "<<buf<<std::endl;

	if(_log){
		_log->flush();
	}
	return exitCode;
}

