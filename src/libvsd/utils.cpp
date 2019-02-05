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

#include <string>
#include <windows.h>
#include <fileapi.h>
#include <iostream>

namespace Utils {

std::wstring getFinalPathNameByHandle(const HANDLE handle)
{
    std::wstring out;
    DWORD size = GetFinalPathNameByHandleW(handle, nullptr, 0, FILE_NAME_NORMALIZED);
    if(size) {
        out.resize(size);
        GetFinalPathNameByHandleW(handle, out.data(), size, FILE_NAME_NORMALIZED);
        /* remove \\?\ */
        out.erase(0, 4);
        return out;
    }
    return {};
}

std::wstring multiByteToWideChar(const std::string &data)
{
        size_t outSize = MultiByteToWideChar(CP_UTF8, MB_USEGLYPHCHARS, data.data(), data.size(), nullptr, 0);
        auto out = std::wstring(outSize, 0);
        MultiByteToWideChar(CP_UTF8, MB_USEGLYPHCHARS, data.data(), data.size(), out.data(), outSize);
        return out;
}

}