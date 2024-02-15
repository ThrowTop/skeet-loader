#include <windows.h>
#include <winternl.h>
#include <tlhelp32.h>
#include <iostream>
#include <filesystem> 
#include <string>

namespace winUtils {
    struct HandleDeleter {
        void operator()(HANDLE handle) const {
            if (handle && handle != INVALID_HANDLE_VALUE) {
                CloseHandle(handle);
            }
        }
    };

    using UniqueHandle = std::unique_ptr<std::remove_pointer<HANDLE>::type, HandleDeleter>;

    inline void CreateConsole() {
        AllocConsole();
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
        FILE* stream = nullptr;
        freopen_s(&stream, "CONOUT$", "w", stdout);
    }

    inline BOOL IsRunningAsAdmin() {
        UniqueHandle hToken;
        BOOL fIsElevated = FALSE;
        TOKEN_ELEVATION elevation;
        DWORD dwSize = sizeof(elevation);

        HANDLE tempHandle = NULL;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tempHandle)) {
            hToken.reset(tempHandle);
            if (GetTokenInformation(hToken.get(), TokenElevation, &elevation, sizeof(elevation), &dwSize)) {
                fIsElevated = elevation.TokenIsElevated;
            }
        }
        return fIsElevated;
    }

    inline DWORD FindProcessId(const std::wstring& processName) {
        PROCESSENTRY32 processInfo;
        processInfo.dwSize = sizeof(processInfo);

        UniqueHandle processesSnapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
        if (processesSnapshot.get() == INVALID_HANDLE_VALUE) {
            std::wcerr << L"Failed to create process snapshot." << std::endl;
            return 0;
        }

        DWORD processID = 0;
        if (Process32First(processesSnapshot.get(), &processInfo)) {
            do {
                if (processName.compare(processInfo.szExeFile) == 0) {
                    processID = processInfo.th32ProcessID;
                    break;
                }
            } while (Process32Next(processesSnapshot.get(), &processInfo));
        }
        else {
            std::wcerr << L"Failed to iterate through the process list." << std::endl;
        }

        return processID;
    }
}