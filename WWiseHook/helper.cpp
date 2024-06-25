#include <pch.h>
#include <string>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <psapi.h>
#include <vector>
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
bool MemoryCompare(const BYTE* bData, const BYTE* bMask, const char* szMask);
BYTE* FindPattern(BYTE* bBase, DWORD dwSize, BYTE* bMask, char* szMask);
BYTE* ScanForPatternInModuleSection(const wchar_t* moduleName, const char* pattern, const char* mask, const char* sectionName);
bool MemoryCompare(const BYTE* bData, const BYTE* bMask, const char* szMask)
{
    for (; *szMask; ++szMask, ++bData, ++bMask)
    {
        if (*szMask == 'x' && *bData != *bMask)
            return false;
    }
    return (*szMask) == 0;
}

BYTE* FindPattern(BYTE* bBase, DWORD dwSize, BYTE* bMask, char* szMask)
{
    for (DWORD i = 0; i < dwSize; i++)
    {
        if (MemoryCompare((BYTE*)(bBase + i), bMask, szMask))
        {
            return (BYTE*)(bBase + i);
        }
    }
    return nullptr;
}

BYTE* GetPatternAddressInModuleSection(const wchar_t* moduleName, const char* pattern, const char* mask, const char* sectionName)
{
    // Get the module handle
    HMODULE hModule = GetModuleHandle(moduleName);
    if (hModule == NULL)
    {
        std::cerr << "Module not found!" << std::endl;
        return nullptr;
    }

    // Get module information
    MODULEINFO modInfo = { 0 };
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO)))
    {
        std::cerr << "Failed to get module information!" << std::endl;
        return nullptr;
    }

    // Get the DOS header
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)modInfo.lpBaseOfDll;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        std::cerr << "Invalid DOS signature!" << std::endl;
        return nullptr;
    }

    // Get the NT headers
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)modInfo.lpBaseOfDll + pDosHeader->e_lfanew);
    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE)
    {
        std::cerr << "Invalid NT signature!" << std::endl;
        return nullptr;
    }

    // Get the section headers
    PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION(pNtHeaders);

    // Iterate through sections to find the specified section
    for (WORD i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++, pSectionHeader++)
    {
        if (strncmp((char*)pSectionHeader->Name, sectionName, IMAGE_SIZEOF_SHORT_NAME) == 0)
        {
            // Found the specified section
            BYTE* sectionBase = (BYTE*)modInfo.lpBaseOfDll + pSectionHeader->VirtualAddress;
            DWORD sectionSize = pSectionHeader->SizeOfRawData;

            // Scan for the pattern in the specified section
            return FindPattern(sectionBase, sectionSize, (BYTE*)pattern, (char*)mask);
        }
    }

    std::cerr << "Section not found!" << std::endl;
    return nullptr;
}
uint8_t* GetAddress()
{
    const wchar_t* moduleName = L"UserAssembly.dll";
    const char* pattern = "\x48\x89\x5c\x24\x10\x57\x48\x83\xec\x20\x48\x8b\xf9\xbb\xc5\x9d\x1c\x81";
    const char* mask = "xxxxxxxxxxxxxxxxxx";
    const char* sectionName = "il2cpp"; // Specify the section name to search in

    return GetPatternAddressInModuleSection(moduleName, pattern, mask, sectionName);
}