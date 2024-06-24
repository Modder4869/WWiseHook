#include <pch.h>
#include <string>
#include <windows.h>
#include <iostream>
#include <fstream>
using namespace std;
static std::wstring ExtractDirectory(const std::wstring& path) {
    size_t lastSlashPos = path.find_last_of(L"\\/");
    if (lastSlashPos != std::wstring::npos) {
        return path.substr(0, lastSlashPos);
    }
    return L"";
}
static std::wstring JoinPath(const std::wstring& dir, const std::wstring& filename) {
    return dir + L"\\" + filename;
}
static std::wstring GetModulePath(const std::wstring& moduleName) {
    HMODULE hModule = GetModuleHandle(moduleName.c_str());
    if (!hModule) {
        std::wcerr << L"Failed to get handle for module: " << moduleName << std::endl;
        return L"";
    }

    wchar_t buffer[MAX_PATH];
    if (GetModuleFileName(hModule, buffer, MAX_PATH) == 0) {
        std::wcerr << L"Failed to get module file name for: " << moduleName << std::endl;
        return L"";
    }

    return std::wstring(buffer);
}
void WriteOutputToFile(const std::wstring& output) {
        static std::wstring modulePath;

        if (modulePath.empty()) {
        modulePath = GetModulePath(L"WWiseHook.dll");         if (modulePath.empty()) {
            std::wcerr << L"Failed to get module path." << std::endl;
            return;
        }
    }

        std::wstring directory = ExtractDirectory(modulePath);
    if (directory.empty()) {
        std::wcerr << L"Failed to extract directory from module path." << std::endl;
        return;
    }

        std::wstring logFilePath = JoinPath(directory, L"WWise_log.txt");
    std::wofstream outFile(logFilePath, std::ios_base::app);

    if (outFile.is_open()) {
        outFile << output << std::endl;
        outFile.close();
    }
    else {
        std::wcerr << L"Failed to open log file: " << logFilePath << std::endl;
    }
}
