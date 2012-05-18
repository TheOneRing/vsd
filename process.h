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


    class LIBVSD_EXPORT Process
    {
    public:
        typedef void readyReadANSI(char *data);
        typedef void readyReadUTF8(wchar_t *data);

        Process(wchar_t *program,wchar_t * arguments);
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

        HANDLE m_stdOut;
        STARTUPINFO m_si; 
        PROCESS_INFORMATION m_pi; 


    };
}

#endif //PROCESS_H
