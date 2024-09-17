#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <string_view>
#include <iostream>

namespace memory
{
    inline HANDLE gHandle;
    inline uintptr_t pID;

    // Function to get process ID by name
    uintptr_t GetProcID(const wchar_t* process)
    {
        ::PROCESSENTRY32 entry = { };
        entry.dwSize = sizeof(::PROCESSENTRY32);

        HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            std::cerr << "[-] Failed to create snapshot of processes" << std::endl;
            return 0;
        }

        uintptr_t procID = 0;
        while (::Process32Next(snapshot, &entry))
        {
            if (!_wcsicmp(process, entry.szExeFile))
            {
                procID = entry.th32ProcessID;
                std::cout << "[+] Process '" << process << "' found with PID: " << procID << std::endl;
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
            std::cerr << "[-] Failed to create snapshot of modules" << std::endl;
            return 0;
        }

        uintptr_t baseAddress = 0;
        while (::Module32Next(snapshot, &entry))
        {
            if (!_wcsicmp(module, entry.szModule))
            {
                baseAddress = reinterpret_cast<uintptr_t>(entry.modBaseAddr);
                std::cout << "[+] Module '" << module << "' found at base address: " << std::hex << baseAddress << std::endl;
                break;
            }
        }

        ::CloseHandle(snapshot);
        return baseAddress;
    }

    // Template function to read process memory

    template <typename T> T Read(uintptr_t address)
    {
        T ret;
        ReadProcessMemory(gHandle, (LPCVOID)address, &ret, sizeof(T), nullptr);
        return ret;
    }

    // Template function to write process memory
    template <typename T> bool Write(uintptr_t address, T value)
    {
        return WriteProcessMemory(gHandle, (LPVOID)address, &value, sizeof(T), nullptr);
    }
}

// Function to get handle to process by name
HANDLE GetHandle(const std::string_view processName) noexcept
{
    ::PROCESSENTRY32 entry = { };
    entry.dwSize = sizeof(::PROCESSENTRY32);

    HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "[-] Failed to create snapshot of processes" << std::endl;
        return nullptr;
    }

    HANDLE processHandle = nullptr;
    while (::Process32Next(snapshot, &entry))
    {
        if (!processName.compare(entry.szExeFile))
        {
            DWORD processId = entry.th32ProcessID;
            processHandle = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

            if (processHandle == NULL) {
                std::cerr << "[-] Failed to open process: " << processName << std::endl;
            }
            else {
                std::cout << "[+] Process '" << processName << "' found and handle opened" << std::endl;
            }
            break;
        }
    }

    ::CloseHandle(snapshot);
    return processHandle;
}

// Function to get the base address of a module by name
std::uintptr_t GetModuleAddress(HANDLE processHandle, DWORD processId, const std::string_view moduleName) noexcept
{
    ::MODULEENTRY32 entry = { };
    entry.dwSize = sizeof(::MODULEENTRY32);

    HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);

    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "[-] Failed to create snapshot of modules" << std::endl;
        return 0;
    }

    std::uintptr_t moduleBase = 0;
    while (::Module32Next(snapshot, &entry))
    {
        if (!moduleName.compare(entry.szModule))
        {
            moduleBase = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
            std::cout << "[+] Module '" << moduleName << "' found at address: " << std::hex << moduleBase << std::endl;
            break;
        }
    }

    ::CloseHandle(snapshot);
    return moduleBase;
}
