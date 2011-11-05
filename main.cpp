
#include <windows.h>
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

void initArgs(wchar_t *out, wchar_t *in[],int len){
    for(int i=2;i<len;++i){
        wcscpy(out,_T(" \""));
        out+=2;
        wcscpy(out,in[i]);
        out += wcslen(in[i]);
        wcscpy(out++,_T("\""));
    }
    wcscpy(out++,_T("\0"));
}

int _tmain(int argc, _TCHAR* argv[])
{
    if(argc<2)
        return -1;
    wchar_t  *program = argv[1];
    wchar_t  *arguments = new wchar_t[MAX_PATH *2];
    initArgs(arguments,argv,argc);

    Process p(program,arguments);
    delete [] arguments;


    p.setANSICallback(readyReadANSI);
    p.setUTF8Callback(readyReadUTF8);

    unsigned long exitCode = p.run();
    char buf[20];
    ltoa(exitCode,buf,10);
    std::wcout<<p.program()<<p.arguments()<<_T(" Exited with status: ")<<buf<<std::endl;


    return exitCode;
}

