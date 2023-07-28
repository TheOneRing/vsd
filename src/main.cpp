/*
    VSD prints debugging messages of applications and their
    sub-processes to console and supports logging of their output.
    Copyright (C) 2012-2020  Hannah von Reth <vonreth@kde.org>


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

#include "libvsd/vsdprocess.h"
#include "libvsd/vsdchildprocess.h"
#include "libvsd/utils.h"

#include "3dparty/nlohmann/json.hpp"

#include <windows.h>
#include <shellapi.h>

#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <signal.h>
#include <mutex>
#include <chrono>
#include <regex>
#include <clocale>
#include <filesystem>

#include <iostream>
#include <io.h>
#include <ios>
#include <fcntl.h>
#include <codecvt>

namespace {
constexpr bool iseol(wchar_t c)
{
    return c == L'\n' || c == L'\r';
}

inline std::wstring rtrim(std::wstring s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !iseol(ch);
    }).base(),
        s.end());
    return s;
}

std::filesystem::path configPath()
{
    std::filesystem::path path(Utils::getModuleName(GetCurrentProcess(), nullptr));
    return path.parent_path() / L"vsd.conf";
}

class ColorStream
{
public:
    ColorStream() = default;
    virtual ~ColorStream() {};
    virtual ColorStream &setColor(WORD color) = 0;
    virtual ColorStream &operator<<(const std::wstring_view &) = 0;

    ColorStream &operator<<(int i)
    {
        return *this << std::to_wstring(i);
    };
};

class ColorGroupoStream : public ColorStream
{
public:
    ~ColorGroupoStream()
    {
        for (const auto str : m_streams) {
            delete str;
        }
        m_streams.clear();
    }
    ColorStream &setColor(WORD color) override
    {
        for (const auto str : m_streams) {
            str->setColor(color);
        }
        return *this;
    }

    void addStream(ColorStream *stream)
    {
        m_streams.push_back(stream);
    }

    ColorStream &operator<<(const std::wstring_view &x) override
    {
        for (const auto str : m_streams) {
            *str << x;
        }
        return *this;
    }

private:
    std::vector<ColorStream *> m_streams;
};

class ColorOutStream : public ColorStream
{
public:
    ColorOutStream(HANDLE hout)
        : m_hout(hout)
    {
    }
    virtual ColorStream &setColor(WORD color) override
    {
        SetConsoleTextAttribute(m_hout, color);
        return *this;
    };

    ColorStream &operator<<(const std::wstring_view &x) override
    {
        std::wcout << x;
        return *this;
    }

private:
    HANDLE m_hout;
};


class SimpleFileStream : public ColorStream
{
public:
    SimpleFileStream(const std::filesystem::path &name)
    {
        m_out.open(name, std::ios::out | std::ios::binary);
        // unsigned char utf8BOM[] = {0xEF,0xBB,0xBF};
        // m_out.write((wchar_t*)utf8BOM,sizeof(utf8BOM)/2);
#pragma warning(disable : 4996)
        const std::locale utf8_locale = std::locale(std::locale(),
            new std::codecvt_utf8_utf16<wchar_t, 0x10ffff, std::codecvt_mode(std::consume_header | std::generate_header)>);
#pragma warning(default : 4996)
        m_out.imbue(utf8_locale);
    }
    virtual ~SimpleFileStream()
    {
        m_out.close();
    }

    virtual ColorStream &setColor(WORD) override
    {
        return *this;
    };

    ColorStream &operator<<(const std::wstring_view &x) override
    {
        m_out << x;
        return *this;
    }

protected:
    std::wofstream m_out;
};

class ColorFileStream : public SimpleFileStream
{
public:
    ColorFileStream(const std::filesystem::path &name, const std::wstring &program, const std::wstring &arguments)
        : SimpleFileStream(name)
    {
        m_out << "<!DOCTYPE html>\n"
              << "<html>\n"
              << "<head>\n"
              << "<meta charset=\"UTF-8\" />\n"
              << "<title>VSD " << program << " " << arguments << "</title>\n"
              << "</head>\n"
              << "<body>"
              << "<p style=\"color:blue\">";
    }
    ~ColorFileStream()
    {
        m_out << "</body>\n\n</html>\n";
    }

    virtual ColorStream &setColor(WORD color) override
    {
        m_out << "</p><p style=\"color:";
        switch (color) {
        case FOREGROUND_BLUE:
        case FOREGROUND_BLUE | FOREGROUND_INTENSITY:
            m_out << "blue";
            break;
        case FOREGROUND_GREEN:
        case FOREGROUND_GREEN | FOREGROUND_INTENSITY:
            m_out << "green";
            break;
        case FOREGROUND_RED:
        case FOREGROUND_RED | FOREGROUND_INTENSITY:
            m_out << "red";
            break;
        default:
            m_out << "black";
        }
        m_out << "\">";
        return *this;
    };

    ColorStream &operator<<(const std::wstring_view &x) override
    {
        static std::wregex regex(L"[\\r|\\r\\n]");
        m_out << std::regex_replace(x.data(), regex, L"</br>");
        return *this;
    }
};
}

using namespace libvsd;

void printHelp()
{
    std::wcout << L"Usage: vsd TARGET_APPLICATION [ARGUMENTS] [OPTIONS]" << std::endl
               << L"Options:" << std::endl
               << L"--vsd-separate-error \t\t Separate stderr and stdout to identify stderr messages" << std::endl
               << L"--vsd-log logFile \t\t Write the logFile in colored html" << std::endl
               << L"--vsd-log-plain logFile \t Write a log plaintext to logFile" << std::endl
               << L"--vsd-all\t\t\t Debug also all processes created by TARGET_APPLICATION" << std::endl
               << L"--vsd-debug-dll\t\t\t Debugg dll loading" << std::endl
               << L"--vsd-log-dll\t\t\t Log dll loading" << std::endl
               << L"--vsd-no-console\t\t Don't log to console" << std::endl
               << L"--help \t\t\t\t Print this help" << std::endl
               << L"--version\t\t\t Print version and copyright information" << std::endl;
    exit(0);
}

void printVersion()
{
    std::wcout << L"VSD version 0.9.0" << std::endl
               << std::endl
               << L"Copyright (C) 2012-2023  Hannah von Reth <vonreth@kde.org>" << std::endl
               << std::endl
               << L"VSD is free software: you can redistribute it and/or modify" << std::endl
               << L"it under the terms of the GNU Lesser General Public License as published by" << std::endl
               << L"the Free Software Foundation, either version 3 of the License, or" << std::endl
               << L"(at your option) any later version." << std::endl;
    exit(0);
}

class VSDImp : public VSDClient
{
public:
    VSDImp(wchar_t *in[], int len)
    {
        std::wstring program(in[1]);
        std::wstringstream arguments;
        nlohmann::json config = nlohmann::json::parse("{}");

        const std::filesystem::path confFile = configPath();
        if (std::filesystem::exists(confFile)) {
            std::ifstream stream(confFile);
            stream >> config;
        }
        bool debug_dll = config.value("debugDllLoading", false);
        m_logDll = config.value("logDllLoading", false);
        bool withSubProcess = config.value("attachSubprocess", false);

        std::filesystem::path logFile;
        bool htmlLog = config.value("logHtml", true);
        m_channels = config.value("mergeChannels", true) ? VSDProcess::ProcessChannelMode::MergedChannels : VSDProcess::ProcessChannelMode::SeperateChannels;


        for (int i = 1; i < len; ++i) {
            std::wstring arg(in[i]);
            if (arg == L"--vsd-separate-error") {
                m_channels = VSDProcess::ProcessChannelMode::SeperateChannels;
            } else if (arg == L"--vsd-debug-dll") {
                debug_dll = true;
            } else if (arg == L"--vsd-log-dll") {
                m_logDll = true;
            } else if (arg == L"--vsd-log") {
                htmlLog = true;
                if (i + 1 < len) {
                    logFile = in[++i];
                } else {
                    printHelp();
                }
            } else if (arg == L"--vsd-log-plain") {
                htmlLog = false;
                if (i + 1 < len) {
                    logFile = in[++i];
                } else {
                    printHelp();
                }
            } else if (arg == L"--vsd-all") {
                withSubProcess = true;
            } else if (arg == L"--vsd-no-console") {
                m_noOutput = true;
            } else if (i == 1) {
                if (arg == L"--help") {
                    printHelp();
                } else if (arg == L"--version") {
                    printVersion();
                }
            } else if (i > 1) {
                arguments << "\"" << arg << "\" ";
            }
        }

        if (!m_noOutput) {
            m_hout = GetStdHandle(STD_OUTPUT_HANDLE);
            GetConsoleScreenBufferInfo(m_hout, &m_consoleSettings);
            m_out.addStream(new ColorOutStream(m_hout));
        }

        if (!logFile.empty()) {
            if (htmlLog) {
                m_out.addStream(new ColorFileStream(logFile, program, arguments.str()));
            } else {
                m_out.addStream(new SimpleFileStream(logFile));
            }
        }
        m_out.setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY) << program << L" " << arguments.str() << L"\n";

        m_process = new VSDProcess(program, arguments.str(), this);
        m_process->debugDllLoading(debug_dll);
        m_process->debugSubProcess(withSubProcess);
    }

    ~VSDImp()
    {
        delete m_process;
        SetConsoleTextAttribute(m_hout, m_consoleSettings.wAttributes);
        CloseHandle(m_hout);
    }


    inline void run()
    {
        m_exitCode = m_process->run(m_channels);
        m_out.setColor(0) << L"\n";
    }

    inline std::wstring getTimestamp(const std::chrono::high_resolution_clock::duration &time)
    {
        std::wstringstream out;
        out << std::chrono::duration_cast<std::chrono::hours>(time).count() << ":"
            << std::chrono::duration_cast<std::chrono::minutes>(time).count() % 60 << ":"
            << std::chrono::duration_cast<std::chrono::seconds>(time).count() % 60 << ":"
            << std::chrono::duration_cast<std::chrono::milliseconds>(time).count() % 1000;
        return out.str();
    }

    inline void writeStdout(const std::wstring &data)
    {
        m_out.setColor(m_consoleSettings.wAttributes) << data;
    }

    inline void writeErr(const std::wstring &data)
    {
        m_out.setColor(FOREGROUND_RED | FOREGROUND_INTENSITY) << data;
    }

    inline void writeDebug(const VSDChildProcess *process, const std::wstring &data)
    {
        m_out.setColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY) << process->name() << L"(" << process->id() << L"): " << rtrim(data) << L"\n";
    }

    void writeDllLoad(const VSDChildProcess *process, const std::wstring &data, bool loading)
    {
        if (m_logDll) {
            m_out.setColor(FOREGROUND_GREEN) << process->name() << L"(" << process->id() << L"): " << (loading ? L"Loading: " : L"Unloading: ") << data << L"\n";
        }
    }

    inline void processStarted(const VSDChildProcess *process)
    {
        m_out.setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY)
            << L"Process Created: " << process->path().wstring() << L" [" << process->arguments() << L"] (" << process->id() << L")\n";
    }

    inline void processStopped(const VSDChildProcess *process)
    {
        m_out.setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY)
            << L"Process Stopped: "
            << process->path().wstring()
            << L" (" << process->id() << L")";
        if (!process->error().empty()) {
            m_out << L" Error: "
                  << process->error();
        }
        std::wstringstream exitCode;
        exitCode << std::hex << std::showbase << process->exitCode() << std::dec;
        m_out << L" With exit Code: "
              << exitCode.str()
              << L" After: "
              << getTimestamp(process->time())
              << L"\n";
    }

    inline void stop()
    {
        m_process->stop();
    }

    int m_exitCode = 0;

private:
    ColorGroupoStream m_out;
    VSDProcess *m_process;
    HANDLE m_hout = INVALID_HANDLE_VALUE;
    CONSOLE_SCREEN_BUFFER_INFO m_consoleSettings;
    bool m_noOutput = false;
    bool m_logDll = false;
    VSDProcess::ProcessChannelMode m_channels = VSDProcess::ProcessChannelMode::MergedChannels;
};


static VSDImp *vsdimp = nullptr;

void sighandler(int sig)
{
    (void)sig;
    if (vsdimp != nullptr) {
        vsdimp->stop();
    }
}


int main()
{
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);

    int argc;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc < 2) {
        printHelp();
    }

    vsdimp = new VSDImp(argv, argc);

    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);

    vsdimp->run();
    int ret = vsdimp->m_exitCode;
    delete vsdimp;
    vsdimp = nullptr;
    return ret;
}
