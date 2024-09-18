#pragma once

#include <Windows.h>
#include <TlHelp32.h>

//thanks nurmolo :3

namespace memory
{
    inline HANDLE gHandle = nullptr;
    inline uintptr_t pID = 0;

    // Function to get process ID by name
    uintptr_t GetProcID(const wchar_t* process)
    {
        ::PROCESSENTRY32 entry = { };
        entry.dwSize = sizeof(::PROCESSENTRY32);

        HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return 0;
        }

        uintptr_t procID = 0;
        while (::Process32Next(snapshot, &entry))
        {
            if (!_wcsicmp(process, entry.szExeFile))
            {
                procID = entry.th32ProcessID;
                break;
            }
        }

        ::CloseHandle(snapshot);
        return procID;
    }

    // Function to get module base address
    uintptr_t GetModuleBaseAddress(uintptr_t procID, const wchar_t* module)
    {
        ::MODULEENTRY32 entry = { };
        entry.dwSize = sizeof(::MODULEENTRY32);

        HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procID);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return 0;
        }

        uintptr_t baseAddress = 0;
        while (::Module32Next(snapshot, &entry))
        {
            if (!_wcsicmp(module, entry.szModule))
            {
                baseAddress = reinterpret_cast<uintptr_t>(entry.modBaseAddr);
                break;
            }
        }

        ::CloseHandle(snapshot);
        return baseAddress;
    }

    // Template function to read process memory
    template <typename T>
    T Read(uintptr_t address)
    {
        T ret;
        ReadProcessMemory(gHandle, reinterpret_cast<LPCVOID>(address), &ret, sizeof(T), nullptr);
        return ret;
    }

    // Template function to write process memory
    template <typename T>
    bool Write(uintptr_t address, T value)
    {
        return WriteProcessMemory(gHandle, reinterpret_cast<LPVOID>(address), &value, sizeof(T), nullptr);
    }
}
