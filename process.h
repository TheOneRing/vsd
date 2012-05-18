#ifndef PROCESS_H
#define PROCESS_H

#ifndef LIBVSD_EXPORT
    #ifdef BUILDING_LIBVSD
        #define LIBVSD_EXPORT __declspec(dllexport)
    #else
      #define LIBVSD_EXPORT __declspec(dllimport)
    #endif
#endif

#include <windows.h>

namespace libvsd {

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


    class LIBVSD_EXPORT Process
    {
    public:
        typedef void readyReadUTF8(wchar_t *data);

        Process(wchar_t *program,wchar_t * arguments,readyReadUTF8 *utf8);
        ~Process();

        int run();
        void setUTF8Callback(readyReadUTF8 *utf8);
        wchar_t *program();
        wchar_t *arguments();

    private:
        void readDebugMSG(DEBUG_EVENT &debugEvent);
        readyReadUTF8 *m_readyUTF8;
        wchar_t *m_program;
        wchar_t *m_arguments;

        STARTUPINFO m_si; 
        PROCESS_INFORMATION m_pi; 
		Pipe m_stdout;


    };
}

#endif //PROCESS_H
