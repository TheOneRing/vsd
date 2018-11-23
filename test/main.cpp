#include <windows.h>
#include <string>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <clocale>

int main() {
    std::setlocale(LC_ALL, "en_US.UTF-8");
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);

    std::wstring test(L"èéøÞǽлљΣæča\n");

    OutputDebugStringW(test.data());
    std::wcout << L"hello" << std::endl;
    std::wcout << test << std::endl;
    std::wcerr << test << std::endl;
    std::wcout << L"💩" << std::endl;
    OutputDebugStringW(L"💩\n");
    return 0;
}
