
#include <windows.h>
#include <wchar.h>
#include <iostream>
#include <tchar.h>
#include "process.h"

using namespace libvsd;

void readyReadANSI(char * c){
    std::cout<<c;
}

void readyReadUTF8(wchar_t *w){
    std::wcout<<w;
}

int _tmain(int argc, _TCHAR* argv[])
{
    if(argc<2)
        return -1;
    wchar_t  *program = argv[1];
    wchar_t  *arguments = new wchar_t[MAX_PATH *2];
    
    int start=0;     
    for(int i=2;i<argc;++i){
        wcscpy(arguments+ start++,_T(" "));
        wcscpy(arguments+start ,argv[i]);        
        start += (int) wcslen(argv[i]);
    }

    Process p(program,arguments);
    p.setANSICallback(readyReadANSI);
    p.setUTF8Callback(readyReadUTF8);

    unsigned long exitCode = p.run();
    char buf[20];
    ltoa(exitCode,buf,10);
    std::wcout<<p.program()<<p.arguments()<<_T(" Exited with status: ")<<buf<<std::endl;

    
	return exitCode;
}

