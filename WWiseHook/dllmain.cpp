#include "pch.h"
#include <iostream>
#include <sstream>
#include <Windows.h>
#include <thread>
#pragma comment(lib, "MinHook.lib")

#include "MinHook.h"
#include "helper.h"
typedef unsigned long long(__stdcall* GetIDFromString_t)(LPCWCH lpWideCharStr);
typedef unsigned long long(__stdcall* AkShortIdGenerator_t)(LPCWCH lpWideCharStr);

GetIDFromString_t original_GetIDFromString = nullptr;
AkShortIdGenerator_t original_AkShortIdGenerator = nullptr;
std::wstring readUtf16String(const wchar_t* lpWideCharStr) {
    std::wstring result;
    while (*lpWideCharStr) {
        result += *lpWideCharStr++;
    }
    return result;
}
unsigned long long __stdcall hooked_AkShortIdGenerator(LPCWCH lpWideCharStr) {
    try {
        unsigned long long originalReturnValue = original_AkShortIdGenerator(lpWideCharStr);

        // Add 20 bytes to the address
        LPCWCH modifiedAddress = lpWideCharStr + 10;

        // Read the UTF-16 string from the modified address
        std::wstring modifiedString = readUtf16String(modifiedAddress);

        uint32_t returnValueAsUint32 = static_cast<uint32_t>(originalReturnValue);
        std::wostringstream output;
        output << L"UA FNV: " << modifiedString << L" Hash: " << returnValueAsUint32;

        std::wcout << output.str() << std::endl;

        // Uncomment if you want to write to a file
        WriteOutputToFile(output.str());

        return originalReturnValue;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught in hooked_AkShortIdGenerator: " << e.what() << std::endl;
        return 0; // Return an appropriate error value
    }
    catch (...) {
        std::cerr << "Unknown exception caught in hooked_AkShortIdGenerator." << std::endl;
        return 0; // Return an appropriate error value
    }
}
unsigned long long __stdcall hooked_GetIDFromString(LPCWCH lpWideCharStr) {
    unsigned long long originalReturnValue = original_GetIDFromString(lpWideCharStr);

    uint32_t returnValueAsUint32 = static_cast<uint32_t>(originalReturnValue);
    std::wostringstream output;
    output << L"GetIDFromString: " << lpWideCharStr << L" Hash: " << returnValueAsUint32;

    WriteOutputToFile(output.str());

    std::wcout << output.str() << std::endl;
    return originalReturnValue;
}

void CreateOrAttachConsole() {
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        AllocConsole();
    }

    FILE* pFile = nullptr;
    freopen_s(&pFile, "CONOUT$", "w", stdout);
    freopen_s(&pFile, "CONOUT$", "w", stderr);
    freopen_s(&pFile, "CONIN$", "r", stdin);
}

const char* WaitForAnyModule() {
    while (true) {
        if (GetModuleHandleA("aksoundengine.dll") != NULL) {
            return "aksoundengine.dll";
        }
        if (GetModuleHandleA("Mmoron.dll") != NULL) {
            return "Mmoron.dll";
        }
        Sleep(100);
    }
}
const char* WaitForUA() {
    while (true) {
        if (GetModuleHandleA("UserAssembly.dll") != NULL) {
            printf("Base address of %s: %p\n", "UA", GetModuleHandleA("UserAssembly.dll"));
            return "UA";
        }

        Sleep(100);
    }
}
bool ThreadFunction() {
    const char* moduleName = WaitForAnyModule();
    if (moduleName == NULL) {
        std::cerr << "Failed to find loaded module." << std::endl;
        return FALSE;
    }


    std::cout << "Found loaded module: " << moduleName << std::endl;

    if (MH_Initialize() != MH_OK) {
        std::cerr << "Failed to initialize MinHook." << std::endl;
        return FALSE;
    }

    if (strcmp(moduleName, "Mmoron.dll") == 0) {
        WaitForUA();
        printf_s("scanning for function address:");
        auto pTargetAddr = GetAddress();
        printf_s("got function address: %p\n", pTargetAddr);
        if (MH_CreateHook(pTargetAddr, &hooked_AkShortIdGenerator, reinterpret_cast<LPVOID*>(&original_AkShortIdGenerator)) != MH_OK) {
            std::cerr << "Failed to create hook." << std::endl;
            return FALSE;
        }
        if (MH_EnableHook(pTargetAddr) != MH_OK) {
            std::cerr << "Failed to enable hook GI." << std::endl;
            return FALSE;
        }


    }
    HMODULE hModuleUnity = GetModuleHandleA(moduleName);
    if (hModuleUnity == NULL) {
        std::cerr << "Failed to get module handle for." << moduleName << std::endl;
        return FALSE;
    }

    LPVOID pTarget = GetProcAddress(hModuleUnity, "?GetIDFromString@SoundEngine@AK@@YAKPEB_W@Z");
    printf_s("Address of GetIDFromString function: %p\n", pTarget);
    if (pTarget == NULL) {
        std::cerr << "Failed to get function address." << std::endl;
        return FALSE;
    }

    printf_s("hooking......");

    if (MH_CreateHook(pTarget, &hooked_GetIDFromString, reinterpret_cast<LPVOID*>(&original_GetIDFromString)) != MH_OK) {
        std::cerr << "Failed to create hook." << std::endl;
        return FALSE;
    }
    if (MH_EnableHook(pTarget) != MH_OK) {
        std::cerr << "Failed to enable hook." << std::endl;
        return FALSE;
    }


    std::cout << "Hook successfully applied." << std::endl;
    return TRUE;
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        CreateOrAttachConsole();
        std::thread(ThreadFunction).detach();
        break;
    }
    case DLL_PROCESS_DETACH:
        MH_Uninitialize();

        FreeConsole();
        break;
    }

    return TRUE;
}
