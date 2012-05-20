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
** License along with FFmpeg; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef VSDPROCESS_H
#define VSDPROCESS_H

#ifndef LIBVSD_EXPORT
#ifdef BUILDING_LIBVSD
#define LIBVSD_EXPORT __declspec(dllexport)
#else
#define LIBVSD_EXPORT __declspec(dllimport)
#endif
#endif

#include <windows.h>

namespace libvsd {
	class LIBVSD_EXPORT VSDClient
	{
	public:
		VSDClient();
		~VSDClient();
		virtual void write(const wchar_t *data) = 0;
	};

	class LIBVSD_EXPORT VSDProcess
	{
	public:
		VSDProcess(const wchar_t *program,const wchar_t * arguments,VSDClient *client);
		~VSDProcess();

		int run();
		const wchar_t *program() const;
		const wchar_t *arguments() const;

	private:
		class PrivateVSDProcess;
		PrivateVSDProcess *d;

	};
}

#endif
