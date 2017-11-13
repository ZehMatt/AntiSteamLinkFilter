#include <windows.h>
#include <string>
#include <thread>

#include "BinaryPattern.h"

#ifdef _DEBUG 
#include <conio.h>
#define DEBUG_LOG(fmt, ...) _cprintf(fmt, __VA_ARGS__)
#else 
#define DEBUG_LOG(fmt, ...)
#endif

bool GetRegistryDWORD(HKEY hKey, const char* cszPath, const char* cszKey, DWORD& dwValue)
{
    HKEY hKeyNode = NULL;

    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);

    if (RegOpenKeyExA(hKey, cszPath, 0, KEY_READ, &hKeyNode) != ERROR_SUCCESS)
        return false;

    if (RegQueryValueExA(hKeyNode, cszKey, NULL, &dwType, (LPBYTE)&dwValue, &dwSize) != ERROR_SUCCESS)
        return false;

    if (RegCloseKey(hKeyNode) != ERROR_SUCCESS)
        return false;

    return true;
}

void patchFriendsUIThread()
{
    // Lets wait for the FriendsUI.dll to load.
    HMODULE hFriendUI = nullptr;

    DEBUG_LOG("Waiting for FriendsUI.dll...\n");

    while (hFriendUI == nullptr)
    {
        hFriendUI = GetModuleHandleA("friendsui.dll");
        Sleep(100); // No point in taking all the cpu.
    }

    if (hFriendUI == nullptr)
        return;

    DEBUG_LOG("FriendsUI.dll: %08X\n", hFriendUI);

    DEBUG_LOG("Searching for pattern...\n");

    uintptr_t patternVA = 0;
    if (findPatternInModule(hFriendUI, "75 10 83 C6 ?? 81 FE ?? 00 00 00 72 E5 5F 5E 5B", true, patternVA))
    {
        DEBUG_LOG("Found pattern at %p\n", patternVA);

        DWORD oldProtect = 0;
        if (VirtualProtect((LPVOID)patternVA, 1, PAGE_EXECUTE_READWRITE, &oldProtect) != TRUE)
        {
            MessageBoxA(nullptr, "VirtualProtect failed", "Error", MB_OK);
            return;
        }

        memcpy((void*)patternVA, "\x71", 1);

        DWORD oldProtect2 = 0;
        if (VirtualProtect((LPVOID)patternVA, 1, oldProtect, &oldProtect2) != TRUE)
        {
            MessageBoxA(nullptr, "VirtualProtect failed", "Error", MB_OK);
            return;
        }

        DEBUG_LOG("Patched Link filter!\n");
    }
    else
    {
        DEBUG_LOG("ERROR: Unable to find pattern.\n");
    }
}

typedef int(__cdecl *fpSteamDllMain)(int argc, const char* argv[]);
typedef int(__cdecl *fpSteamDllMainEx)(int argc, const char* argv[], int dummy);

extern "C" __declspec(dllexport) int __cdecl SteamDllMainEx(int argc, const char* argv[], int dummy)
{
#ifdef _DEBUG
    AllocConsole();
#endif 

    DEBUG_LOG("Startup\n");
    DEBUG_LOG("%d, %p, %d\n", argc, argv, dummy);

    HMODULE hSteamUI = LoadLibraryA("SteamUI.dll");
    if (hSteamUI == nullptr)
    {
        MessageBoxA(nullptr, "Unable to load SteamUI.dll library\n", "Error", MB_OK);
        return 0;
    }

    DEBUG_LOG("SteamUI.dll: %08X\n", hSteamUI);

    fpSteamDllMainEx _SteamDllMainEx = reinterpret_cast<fpSteamDllMainEx>(GetProcAddress(hSteamUI, "SteamDllMainEx"));
    if (_SteamDllMainEx == nullptr)
    {
        MessageBoxA(nullptr, "Unable to find procedure 'SteamDllMainEx' in SteamUI.dll\n", "Error", MB_OK);
        return 0;
    }

    DEBUG_LOG("SteamUI.SteamDllMain: %08X\n", hSteamUI);

    std::thread patchingThread(patchFriendsUIThread);

    // Run the UI
    int res = _SteamDllMainEx(argc, argv, dummy);

    if (patchingThread.joinable())
    {
        DEBUG_LOG("Waiting for thread to finish...\n");
        patchingThread.join();
    }

    // In case steam wants to restart.
    DWORD dwRestart = 0;
    if (GetRegistryDWORD(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "Restart", dwRestart) && dwRestart == 1)
    {
        DEBUG_LOG("Steam requested restart, executing process...\n");

        std::string strParams;

        // Reconstruct parameter.
        for (int i = 1; i < argc; i++)
        {
            bool addTags = false;
            for (size_t n = 0; n < strlen(argv[i]); n++)
            {
                if (isspace(argv[i][n]))
                {
                    addTags = true;
                    break;
                }
            }

            if (addTags)
            {
                strParams += "\"";
                strParams += argv[i];
                strParams += "\"";
            }
            else
                strParams += argv[i];

            if (i + 1 < argc)
                strParams += " ";
        }

        // Restart
        ShellExecuteA(NULL, "open", argv[0], strParams.c_str(), NULL, SW_SHOW);
    }

    FreeLibrary(hSteamUI);

    return res;
}