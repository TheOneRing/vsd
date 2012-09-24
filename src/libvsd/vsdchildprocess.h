#ifndef VSDCHILDPROCESS_H
#define VSDCHILDPROCESS_H

#include "vsd_exports.h"

#include <windows.h>
#include <wchar.h>

namespace libvsd
{

class LIBVSD_EXPORT VSDChildProcess
{
public:
    VSDChildProcess(const unsigned long id,const HANDLE fileHandle);
    ~VSDChildProcess();

    const HANDLE& handle() const;
    const wchar_t *path() const;
    const wchar_t *name() const;
    const double time() const;
    const unsigned long id() const;

private:
    long findLastBackslash(const wchar_t *in);

    HANDLE m_handle;
    wchar_t* m_path;
    wchar_t* m_name;
    unsigned long m_id;
    time_t m_startTime;

};

}

#endif // VSDCHILDPROCESS_H
