/*
    VSD prints debugging messages of applications and their
    sub-processes to console and supports logging of their output.
    Copyright (C) 2018  Hannah von Reth <vonreth@kde.org>


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

#include "utils.h"

#include <comdef.h>
#include <fileapi.h>
#include <iostream>
#include <string>
#include <windows.h>
#include <sstream>

namespace Utils {

std::wstring getFinalPathNameByHandle(const HANDLE handle)
{
    std::wstring out;
    DWORD size = GetFinalPathNameByHandleW(handle, nullptr, 0, FILE_NAME_NORMALIZED);
    if (size) {
        out.resize(size);
        if (!GetFinalPathNameByHandleW(handle, out.data(), size, FILE_NAME_NORMALIZED)) {
            return L"getFinalPathNameByHandle Failed! " + formatError(GetLastError());
        }
        /* remove \\?\ */
        out.erase(0, 4);
        trimNull(out);
        return out;
    }
    return L"getFinalPathNameByHandle Failed! " + formatError(GetLastError());
}

std::wstring multiByteToWideChar(const std::string &data)
{
    const size_t outSize = MultiByteToWideChar(CP_UTF8, MB_USEGLYPHCHARS, data.data(), static_cast<int>(data.size()), nullptr, 0);
    auto out = std::wstring(outSize, 0);
    MultiByteToWideChar(CP_UTF8, MB_USEGLYPHCHARS, data.data(), static_cast<int>(data.size()), out.data(), static_cast<int>(outSize));
    return out;
}

std::wstring formatError(unsigned long errorCode)
{
    std::wstringstream out;
    out << L"WindowsError: " << errorCode << " " << _com_error(errorCode).ErrorMessage();
    return out.str();
}

}
