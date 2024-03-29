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

#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <algorithm>
#include <windows.h>

#include "vsd_exports.h"

namespace Utils {
inline void trimNull(std::wstring &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return ch != 0;
    }).base(),
        s.end());
    s.shrink_to_fit();
}

std::wstring getFinalPathNameByHandle(const HANDLE handle);

std::wstring multiByteToWideChar(const std::string &data);

std::wstring formatError(unsigned long errorCode);

LIBVSD_EXPORT std::wstring getModuleName(HANDLE process, HMODULE handle);
};

#endif // UTILS_H
