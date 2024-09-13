#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <string_view>
#include <iostream>

// Get handle to process by process name
HANDLE GetHandle(const std::string_view processName) noexcept
{
    ::PROCESSENTRY32 entry = { };
    entry.dwSize = sizeof(::PROCESSENTRY32);

    HANDLE snapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapShot == INVALID_HANDLE_VALUE) {
        std::cerr << "[-] Failed to create snapshot of processes" << std::endl;
        return nullptr;
    }

    HANDLE processHandle = nullptr;
    while (::Process32Next(snapShot, &entry))
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

    ::CloseHandle(snapShot);
    return processHandle;
}

// Returns the base address of a module by name
std::uintptr_t GetModuleAddress(HANDLE processHandle, DWORD processId, const std::string_view moduleName) noexcept
{
    ::MODULEENTRY32 entry = { };
    entry.dwSize = sizeof(::MODULEENTRY32);

    HANDLE snapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);

    if (snapShot == INVALID_HANDLE_VALUE) {
        std::cerr << "[-] Failed to create snapshot of modules" << std::endl;
        return 0;
    }

    std::uintptr_t moduleBase = 0;
    while (::Module32Next(snapShot, &entry))
    {
        if (!moduleName.compare(entry.szModule))
        {
            moduleBase = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
            std::cout << "[+] Module '" << moduleName << "' found at address: " << std::hex << moduleBase << std::endl;
            break;
        }
    }

    ::CloseHandle(snapShot);
    return moduleBase;
}

// Read process memory template function
template <typename T>
T read(HANDLE processHandle, const std::uintptr_t address) noexcept
{
    T value = { };
    if (!::ReadProcessMemory(processHandle, reinterpret_cast<const void*>(address), &value, sizeof(T), NULL)) {
        std::cerr << "[-] Failed to read memory at address: " << std::hex << address << std::endl;
    }
    return value;
}

// Write process memory template function
template <typename T>
bool write(HANDLE processHandle, const std::uintptr_t address, const T& value) noexcept
{
    if (!::WriteProcessMemory(processHandle, reinterpret_cast<void*>(address), &value, sizeof(T), NULL)) {
        std::cerr << "[-] Failed to write memory at address: " << std::hex << address << std::endl;
        return false;
    }
    return true;
}
