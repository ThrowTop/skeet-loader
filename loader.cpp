#include <windows.h>
#include <iostream>
#include <filesystem> 
#include <string>

namespace fs = std::filesystem;

enum class LoaderID {
    CSGO = 1,
    RUST = 1 << 4,
    CS2 = 1 << 7
};

inline void RunLoader(DWORD processID, LoaderID loadValue) {
    std::wstring loaderDirectory = fs::current_path().wstring();
    std::wstring loaderExeName = fs::path(__wargv[0]).filename().wstring();
    bool started = false;

    std::wcout << L"Running loader" << std::endl;
    for (const auto& entry : fs::directory_iterator(loaderDirectory)) {
        if (entry.is_regular_file() && entry.path().extension() == L".exe" && entry.path().filename() != loaderExeName) {
            STARTUPINFO si{};
            PROCESS_INFORMATION pi{};
            si.cb = sizeof(si);

            std::wstring command = L"\"" + entry.path().wstring() + L"\" --pid=" + std::to_wstring(processID) + L" --load=" + std::to_wstring(static_cast<int>(loadValue));
            std::wcout << L"Executing command: " << command << std::endl;
            started = true;
            if (CreateProcessW(NULL, &command[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            break;
        }
    }
    if (!started) {
        MessageBoxA(NULL,
            "Please make sure this program is located in the same directory as the Gamesense loader",
            "Can not find loader",
            MB_OK | MB_ICONEXCLAMATION);
    }
        
}
