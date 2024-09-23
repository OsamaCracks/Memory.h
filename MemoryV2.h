#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <string_view>
#include <iostream>

class Memory
{
private:
    std::uintptr_t processId = 0;
    void* processHandle = nullptr;
    bool debug = false;

public:
    Memory(const std::string_view processName, bool debugMode = false) noexcept 
        : debug(debugMode)
    {
        ::PROCESSENTRY32 entry = {};
        entry.dwSize = sizeof(entry);

        const auto snapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapShot == INVALID_HANDLE_VALUE) {
            if (debug) std::cerr << "Failed to create process snapshot.\n";
            return;
        }

        while (::Process32Next(snapShot, &entry)) {
            if (processName == entry.szExeFile) {
                processId = entry.th32ProcessID;
                processHandle = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
                if (!processHandle && debug) {
                    std::cerr << "Failed to open process handle.\n";
                }
                break;
            }
        }

        ::CloseHandle(snapShot);
    }

    ~Memory()
    {
        if (processHandle) {
            ::CloseHandle(processHandle);
        }
    }

    // Returns the base address of a module by name
    std::uintptr_t GetModuleAddress(const std::string_view moduleName) const noexcept
    {
        ::MODULEENTRY32 entry = {};
        entry.dwSize = sizeof(entry);

        const auto snapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
        if (snapShot == INVALID_HANDLE_VALUE) {
            if (debug) std::cerr << "Failed to create module snapshot.\n";
            return 0;
        }

        std::uintptr_t result = 0;

        while (::Module32Next(snapShot, &entry)) {
            if (moduleName == entry.szModule) {
                result = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
                break;
            }
        }

        ::CloseHandle(snapShot);
        return result;
    }

    // Read process memory
    template <typename T>
    T Read(const std::uintptr_t address) const noexcept
    {
        T value{};
        if (!::ReadProcessMemory(processHandle, reinterpret_cast<const void*>(address), &value, sizeof(T), nullptr)) {
            if (debug) std::cerr << "Failed to read memory at address: " << address << '\n';
        }
        return value;
    }

    // Write process memory
    template <typename T>
    void Write(const std::uintptr_t address, const T& value) const noexcept
    {
        if (!::WriteProcessMemory(processHandle, reinterpret_cast<void*>(address), &value, sizeof(T), nullptr)) {
            if (debug) std::cerr << "Failed to write memory at address: " << address << '\n';
        }
    }
};
