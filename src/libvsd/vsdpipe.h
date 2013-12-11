/*
    VSD prints debugging messages of applications and their
    sub-processes to console and supports logging of their output.
    Copyright (C) 2013  Patrick von Reth <vonreth@kde.org>


    VSD is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VSD is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with VSD.  If not, see <http://www.gnu.org/licenses/>.
    */

#ifndef VSDPIPE_H
#define VSDPIPE_H

#include <windows.h>
#include <iostream>

//inspired by https://qt.gitorious.org/qt-labs/jom/blobs/master/src/jomlib/process.cpp
class VSDPipe
{
public:
    VSDPipe() :
        hWrite(INVALID_HANDLE_VALUE),
        hRead(INVALID_HANDLE_VALUE)
    {
        ZeroMemory(&overlapped, sizeof(overlapped));
    }


    ~VSDPipe()
    {
        if (hWrite != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hWrite);
        }
        if (hRead != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hRead);
        }
    }

    inline bool isValid() const
    {
        return hRead != INVALID_HANDLE_VALUE || hWrite != INVALID_HANDLE_VALUE;
    }

    inline bool operator ==(const VSDPipe& p) const
    {
        return hWrite == p.hWrite && hRead == p.hRead;
    }

    inline bool setup(SECURITY_ATTRIBUTES *sa)
    {
        size_t maxPipeLen = 256;
        wchar_t *pipeName = new wchar_t[maxPipeLen];
        unsigned int randomValue;
        if (rand_s(&randomValue) != 0)
        {
            randomValue = rand();
        }
        swprintf_s(pipeName, maxPipeLen, L"\\\\.\\pipe\\vsd-%X", randomValue);

        DWORD dwPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS;
        const DWORD dwPipeBufferSize = 1024 * 1024;

        hRead = CreateNamedPipe(pipeName,
                                     PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                                     dwPipeMode,
                                     1,                      // only one pipe instance
                                     0,                      // output buffer size
                                     dwPipeBufferSize,       // input buffer size
                                     0,
                                     sa);
        if (hRead == INVALID_HANDLE_VALUE){
            std::wcerr << L"Creation of the NamedPipe " << pipeName << L" failed " << GetLastError() << std::endl;
            return false;
        }


        hWrite = CreateFile(pipeName,
                                 GENERIC_WRITE,
                                 0,
                                 sa,
                                 OPEN_EXISTING,
                                 FILE_FLAG_OVERLAPPED,
                                 NULL);
        if (hWrite == INVALID_HANDLE_VALUE){
            std::wcerr << L"Creation of the pipe " << pipeName << L" failed " << GetLastError() << std::endl;
            return false;
        }
        ConnectNamedPipe(hRead, NULL);
        return true;
    }


    HANDLE hWrite;
    HANDLE hRead;
    OVERLAPPED overlapped;
};

#endif // VSDPIPE_H
