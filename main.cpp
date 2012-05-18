
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

class VSDImp: public VSDClient{
public:
	VSDImp(wchar_t *out, wchar_t *in[],int len)
		:m_log(NULL)
	{
		wcscpy(out,L"\0");
		for(int i=2;i<len;++i){
			if(wcscmp(in[i],L"--vsdlog")==0){
				if(i+1<=len){
					i++;
					m_log = open_ofstream(in[i++], std::ios::app); 
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


	~VSDImp()
	{
		if(m_log)
			m_log->flush();
	}

	void write(wchar_t *data)
	{
		std::wcout<<data;
		if(m_log)
			*m_log<<data;
		//else
		//{
		//	std::wcout<<L"LOG is null"<<std::endl;
		//}
		SysFreeString(data);
	}

private:
	std::wostream *m_log;
};





int _tmain(int argc,wchar_t *argv[])
{
	if(argc<2)
		return -1;
	wchar_t  *program = argv[1];
	wchar_t  *arguments = new wchar_t[MAX_PATH *2];
	VSDImp vsdimp(arguments,argv,argc);

	VSDProcess p(program,arguments,&vsdimp);
	delete [] arguments;

	unsigned long exitCode = p.run();
	std::wcout<<p.program()<<p.arguments()<<L" Exited with status: "<<exitCode<<std::endl;

	return exitCode;
}

