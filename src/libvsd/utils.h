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

#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <functional>
#include <locale>

namespace VSDUtils
{

static inline void rtrim(std::wstring &s) {
    std::locale loc;
    s.erase(std::find_if(s.rbegin(), s.rend(), [&](wchar_t &t) -> bool { return !std::isspace(t, loc) && t != '\0'; }).base(), s.end());
}


static inline std::wstring toUnicode(char *buff, int len)
{
    std::wstring out;
    wchar_t *wcharBuffer = new wchar_t[len+1];
    std::locale loc;
    std::use_facet< std::ctype<wchar_t> >(loc).widen(buff, buff + len + 1, wcharBuffer);
    out = std::wstring(wcharBuffer, len);
    delete[] wcharBuffer;
    return out;
}
}
#endif // UTILS_H
