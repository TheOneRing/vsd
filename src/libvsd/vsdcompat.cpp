/****************************************************************************
** VSD prints the debug output of a windows GUI apllication to a console window
** Copyright (C) 2012  Patrick von Reth <vonreth@kde.org>
**
**
** VSD is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
* * License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** VSD is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with VSD; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "vsdcompat.h"
#include <time.h>
#ifdef __MINGW64_VERSION_MAJOR

#if __MINGW64_VERSION_MAJOR < 2

errno_t rand_s(unsigned int *in)
{
    static bool initialized = false;
    if(initialized == false){
        srand(time(NULL));
        initialized = true;
    }
    *in = rand();
    return 0;
}
#endif//__MINGW64_VERSION_MAJOR < 2

#endif
