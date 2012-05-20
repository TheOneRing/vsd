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
