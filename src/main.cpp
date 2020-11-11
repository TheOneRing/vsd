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
inline bool iseol(wchar_t c)
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
    std::wstring buf;
    size_t size;
    do {
        buf.resize(buf.size() + 1024);
        size = GetModuleFileNameW(nullptr, const_cast<wchar_t *>(buf.data()),
            static_cast<DWORD>(buf.size()));
    } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);
    buf.resize(size);
    std::filesystem::path path(buf);
    return path.parent_path() / L"vsd.conf";
}
}

using namespace libvsd;

void printHelp()
{
    std::wcout << L"Usage: vsd TARGET_APPLICATION [ARGUMENTS] [OPTIONS]" << std::endl
               << L"Options:" << std::endl
               << L"--vsd-seperate-error \t\t seperate stderr and stdout to identify sterr messages" << std::endl
               << L"--vsd-log logFile \t\t write the logFile in colored html" << std::endl
               << L"--vsd-logplain logFile \t\t write a log to logFile" << std::endl
               << L"--vsd-all\t\t\t debug also all processes created by TARGET_APPLICATION" << std::endl
               << L"--vsd-debug-dll\t\t\t Debugg dll loading" << std::endl
               << L"--vsd-log-dll\t\t\t Log dll loading" << std::endl
               << L"--vsd-nc \t\t\t monochrome output" << std::endl
               << L"--vsd-benchmark #iterations \t VSD won't print the output, a slow terminal would fake the outcome" << std::endl
               << L"--help \t\t\t\t print this help" << std::endl
               << L"--version\t\t\t print version and copyright information" << std::endl;
    exit(0);
}

void printVersion()
{
    std::wcout << L"VSD version 0.8.0" << std::endl
               << std::endl
               << L"Copyright (C) 2012-2020  Hannah von Reth <vonreth@kde.org>" << std::endl
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

        m_html = config.value("logHtml", true);
        m_channels = config.value("mergeChannels", true) ? VSDProcess::ProcessChannelMode::MergedChannels : VSDProcess::ProcessChannelMode::SeperateChannels;


        for (int i = 1; i < len; ++i) {
            std::wstring arg(in[i]);
            if (arg == L"--vsd-seperate-error") {
                m_channels = VSDProcess::ProcessChannelMode::SeperateChannels;
            } else if (arg == L"--vsd-debug-dll") {
                debug_dll = true;
            } else if (arg == L"--vsd-log-dll") {
                m_logDll = true;
            } else if (arg == L"--vsd-log") {
                i = initLog(in, i, len);
            } else if (arg == L"--vsd-logplain") {
                m_html = false;
                i = initLog(in, i, len);
            } else if (arg == L"--vsd-all") {
                withSubProcess = true;
            } else if (arg == L"--vsd-nc") {
                m_colored = false;
            } else if (arg == L"--vsd-benchmark") {
                i = initBenchmark(in, i, len);
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

        m_hout = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleScreenBufferInfo(m_hout, &m_consoleSettings);

        htmlHEADER(program, arguments.str());
        std::wstringstream ws;
        ws << program << L" " << arguments.str() << std::endl;
        print(ws.str(), FOREGROUND_BLUE | FOREGROUND_INTENSITY);

        m_process = new VSDProcess(program, arguments.str(), this);
        m_process->debugDllLoading(debug_dll);
        m_process->debugSubProcess(withSubProcess);
    }

    ~VSDImp()
    {
        std::wcout.flush();
        std::wcerr.flush();

        delete m_process;
        if (m_writeFile) {
            htmlFOOTER();
            m_log.flush();
        }
        SetConsoleTextAttribute(m_hout, m_consoleSettings.wAttributes);
        CloseHandle(m_hout);
    }

    inline int initLog(wchar_t *in[], int pos, int len)
    {
        m_writeFile = true;
        if (pos + 1 < len) {
            std::wstring name(in[++pos]);
            m_log.open(std::string(name.begin(), name.end()).data(), std::ios::out | std::ios::binary);
#pragma warning(disable : 4996)
            const std::locale utf8_locale = std::locale(std::locale(), new std::codecvt_utf8<wchar_t>());
#pragma warning(default : 4996)
            m_log.imbue(utf8_locale);
        } else {
            printHelp();
        }
        return pos;
    }

    inline int initBenchmark(wchar_t *in[], int pos, int len)
    {
        m_noOutput = true;
        if (pos + 1 < len) {
            m_iterations = _wtoi(in[++pos]);
            if (m_iterations == 0) {
                printHelp();
            }
        } else {
            printHelp();
        }
        return pos;
    }


    inline void run()
    {
        std::chrono::high_resolution_clock::duration time(0);
        for (int i = 1; m_run && i <= m_iterations; ++i) {
            m_exitCode = m_process->run(m_channels);
            time += m_process->time();
            if (m_iterations > 1) {
                std::wstringstream ws;
                ws << "\rBenchmark iteration: "
                   << i
                   << ", mean execution time: "
                   << getTimestamp(time / i)
                   << " total execution time "
                   << getTimestamp(time);
                print(ws.str(), FOREGROUND_BLUE | FOREGROUND_INTENSITY, true);
            }
        }
        print(L"\n", 0);
    }

    inline void htmlHEADER(const std::wstring &program, const std::wstring &arguments)
    {
        if (!m_html)
            return;
        m_log << "<!DOCTYPE html>\n"
              << "<html>\n"
              << "<head>\n"
              << "<meta charset=\"UTF-8\" />\n"
              << "<title>VSD " << program << " " << arguments << "</title>\n"
              << "</head>\n"
              << "<body>";
    }

    inline void htmlFOOTER()
    {
        if (!m_html)
            return;
        m_log << "</body>\n\n</html>\n";
    }


    inline void printFile(const std::wstring &data, WORD color)
    {
        if (!m_writeFile) {
            return;
        }
        if (m_html && color != 0) {
            m_log << "<p style=\"color:";
            switch (color) {
            case FOREGROUND_BLUE:
            case FOREGROUND_BLUE | FOREGROUND_INTENSITY:
                m_log << "blue";
                break;
            case FOREGROUND_GREEN:
            case FOREGROUND_GREEN | FOREGROUND_INTENSITY:
                m_log << "green";
                break;
            case FOREGROUND_RED:
            case FOREGROUND_RED | FOREGROUND_INTENSITY:
                m_log << "red";
                break;
            default:
                m_log << "black";
            }
            m_log << "\">";
            static std::wregex regex(L"[\\r|\\r\\n]");
            m_log << std::regex_replace(data, regex, L"</br>") << "</p>\n";
        } else {
            m_log << data;
        }
    }

    std::wstring string_to_hex(const std::wstring &input)
    {
        static const wchar_t *const lut = L"0123456789ABCDEF";
        size_t len = input.length();

        std::wstring output;
        output.reserve(2 * len);
        for (size_t i = 0; i < len; ++i) {
            const wchar_t c = input[i];
            output.push_back(lut[c >> 4]);
            output.push_back(lut[c & 15]);
        }
        return output;
    }

    inline void print(const std::wstring &data, WORD color, bool printAlways = false)
    {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock(mutex);
        if (printAlways || !m_noOutput) {
            if (m_colored)
                SetConsoleTextAttribute(m_hout, color);
            std::wcout << data;
        }
        printFile(data, color);
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
        print(data, m_consoleSettings.wAttributes);
    }

    inline void writeErr(const std::wstring &data)
    {
        print(data, FOREGROUND_RED | FOREGROUND_INTENSITY);
    }

    inline void writeDebug(const VSDChildProcess *process, const std::wstring &data)
    {
        std::wstringstream ws;
        ws << process->name() << L"(" << process->id() << L"): " << rtrim(data) << std::endl;
        print(ws.str(), FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }

    void writeDllLoad(const VSDChildProcess *process, const std::wstring &data, bool loading)
    {
        if (m_logDll) {
            std::wstringstream ws;
            ws << process->name() << L"(" << process->id() << L"): " << (loading ? L"Loading: " : L"Unloading: ") << data << std::endl;
            print(ws.str(), FOREGROUND_GREEN);
        }
    }

    inline void processStarted(const VSDChildProcess *process)
    {
        std::wstringstream ws;
        ws << "Process Created: " << process->path() << " (" << process->id() << ")" << std::endl;
        print(ws.str(), FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    }

    inline void processStopped(const VSDChildProcess *process)
    {
        std::wstringstream ws;
        ws << "Process Stopped: "
           << process->path()
           << " (" << process->id() << ")";
        if (!process->error().empty()) {
            ws << " Error: "
               << process->error();
        }
        ws << " With exit Code: "
           << std::hex << process->exitCode() << std::dec
           << " After: "
           << getTimestamp(process->time())
           << std::endl;
        print(ws.str(), FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    }

    inline void stop()
    {
        m_run = false;
        m_process->stop();
    }

    int m_exitCode = 0;

private:
    VSDProcess *m_process;
    std::wofstream m_log;
    HANDLE m_hout = INVALID_HANDLE_VALUE;
    CONSOLE_SCREEN_BUFFER_INFO m_consoleSettings;
    bool m_colored = true;
    bool m_html = true;
    bool m_noOutput = false;
    int m_iterations = 1;
    bool m_run = true;
    bool m_logDll = false;
    bool m_writeFile = false;
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
