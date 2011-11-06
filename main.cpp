
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
	else
	{
		std::wcout<<L"LOG is null"<<std::endl;
	}
}

void writeANSI(char * c){
	int len = MultiByteToWideChar(CP_ACP, 0, c, -1, NULL,NULL);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_UTF8, 0, c, -1, buf, len);
	writeUTF8(buf);
	delete [] buf;
}



wchar_t *quotate(wchar_t *in){
	wchar_t *out =  new wchar_t[wcslen(in)+2];
	wcscpy(out,L"\"");
	wcscpy(out+1,in);
	wcscpy(out+1+wcslen(in),L"\"");
	return out;
}
void initArgs(wchar_t *out, wchar_t *in[],int len){
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

		wchar_t *tmp = quotate(in[i]);
		wcscpy(out++,L" ");
		wcscpy(out,tmp);
		out += wcslen(tmp);
		delete []tmp;
	}
	wcscpy(out++,L"\0");
}

int main()
{
	int argc;
	wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if(argc<2)
		return -1;
	wchar_t  *program = argv[1];
	wchar_t  *arguments = new wchar_t[MAX_PATH *2];
	initArgs(arguments,argv,argc);

	Process p(program,arguments);
	delete [] arguments;


	p.setANSICallback(writeANSI);
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

