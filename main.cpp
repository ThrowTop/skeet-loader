#include <windows.h>
#include <winternl.h>
#include <tlhelp32.h>
#include <iostream>
#include <filesystem> 
#include <thread>
#include <string>
#include <algorithm>
#include <memory>

#include "winUtils.cpp"
#include "loader.cpp"

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Macros
//#define DEBUG
#define returnif(v) if (v) return v;

namespace fs = std::filesystem;

DWORD getCS2PID();
std::wstring getSteamPath();
void launchCS2(bool);

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
#ifdef DEBUG
    winUtils::CreateConsole();
#endif

    if (!winUtils::IsRunningAsAdmin()) {
        MessageBoxA(NULL,
            "This application requires elevated privileges to function correctly. Please restart it as an administrator.",
            "Administrator Rights Required",
            MB_OK | MB_ICONEXCLAMATION);
        return 1;
    }

    std::wcout << L"Running with elevated privileges." << std::endl;

    DWORD PID = winUtils::FindProcessId(L"cs2.exe");
    if (PID) {
        RunLoader(PID, LoaderID::CS2);
    }
    else {
        launchCS2(false);

        std::this_thread::sleep_for(std::chrono::milliseconds(1250));

        PID = getCS2PID();
        if (PID)
            RunLoader(PID, LoaderID::CS2);
    }

end:
    std::wcout << L"Done!" << std::endl;
#ifdef DEBUG
    system("pause>0");
#endif
    return 0;
}



void launchCS2(bool isInsecure) {
    std::wstring steamExePath = getSteamPath();
    std::wstring appID = L"730";
    std::wstring commandLine = L"-applaunch " + appID + (isInsecure ? L" -insecure" : L"");

    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(si);

    std::wcout << L"Launching CS2:" << std::endl;
    std::wcout << L"Executing command: " << steamExePath << L" " << commandLine << std::endl;
    if (CreateProcessW(steamExePath.c_str(), &commandLine[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        std::wcerr << L"Failed to launch cs2 through Steam." << std::endl;
    }
}

std::wstring getSteamPath() {
    HKEY hKey;
    DWORD dwType = REG_SZ;
    DWORD dwSize = 0;
    long lResult;
    std::wstring steamPath;

    lResult = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", 0, KEY_QUERY_VALUE, &hKey);
    if (lResult == ERROR_SUCCESS) {
        lResult = RegQueryValueExW(hKey, L"SteamPath", NULL, &dwType, NULL, &dwSize);
        if (lResult == ERROR_SUCCESS) {
            wchar_t* buffer = new wchar_t[dwSize / sizeof(wchar_t)];

            lResult = RegQueryValueExW(hKey, L"SteamPath", NULL, &dwType, reinterpret_cast<LPBYTE>(buffer), &dwSize);
            if (lResult == ERROR_SUCCESS) {
                steamPath.assign(buffer, dwSize / sizeof(wchar_t) - 1);
            }
            delete[] buffer;
        }
        RegCloseKey(hKey);
    }
    return steamPath + L"\\Steam.exe";
}


DWORD getCS2PID() {
    auto startTime = std::chrono::steady_clock::now();
    const std::chrono::seconds timeout(120);
    std::chrono::milliseconds delay(10);

    while (true) {
        std::wcout << L"Looking for cs2.exe..." << std::endl;
        DWORD cs2ID = winUtils::FindProcessId(L"cs2.exe");
        returnif(cs2ID);

        auto currentTime = std::chrono::steady_clock::now();
        if (currentTime - startTime > timeout) {
            std::wcout << L"Timeout exceeded. cs2.exe not found." << std::endl;
            return 0;
        }
        std::this_thread::sleep_for(delay);

        delay = min(delay + std::chrono::milliseconds(25), std::chrono::milliseconds(500));
    }
}