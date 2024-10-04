#include <windows.h>
#include <TlHelp32.h>
#include <cstdlib>
#include <tchar.h>
#include <iostream>

namespace memory {
    HANDLE GetHandle(const TCHAR* processName) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return NULL;
        }

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (!Process32First(hSnapshot, &pe32)) {
            CloseHandle(hSnapshot);
            return NULL;
        }

        do {
            if (_tcsicmp(pe32.szExeFile, processName) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
                CloseHandle(hSnapshot);
                return hProcess;
            }
        } while (Process32Next(hSnapshot, &pe32));

        CloseHandle(hSnapshot);
        return NULL;
    }

    uintptr_t GetModuleBaseAddress(DWORD processId, const wchar_t* ModuleTarget) {
        HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
        if (snapshotHandle == INVALID_HANDLE_VALUE) {
            return 0;
        }

        MODULEENTRY32W moduleEntry = { };
        moduleEntry.dwSize = sizeof(MODULEENTRY32W);
        if (Module32FirstW(snapshotHandle, &moduleEntry)) {
            do {
                if (_wcsicmp(moduleEntry.szModule, ModuleTarget) == 0) {
                    uintptr_t baseAddr = reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr);
                    CloseHandle(snapshotHandle);
                    return baseAddr;
                }
            } while (Module32NextW(snapshotHandle, &moduleEntry));
        }

        CloseHandle(snapshotHandle);
        return 0;
    }

    template <typename T>
    bool Write(HANDLE handle, uintptr_t address, const T& value) noexcept {
        SIZE_T bytesWritten;
        return WriteProcessMemory(handle,
            reinterpret_cast<LPVOID>(address),
            &value,
            sizeof(T),
            &bytesWritten)
            && bytesWritten == sizeof(T);
    }

    template <typename T>
    bool Read(HANDLE handle, uintptr_t address, T& value) {  
        SIZE_T bytesRead;
        return ReadProcessMemory(handle,
            reinterpret_cast<LPCVOID>(address),
            &value,
            sizeof(T),
            &bytesRead)
            && bytesRead == sizeof(T);
    }
}
