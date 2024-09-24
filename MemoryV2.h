#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <string_view>
#include <iostream>

namespace Memory
{
private:
    std::uintptr_t processId = 0;
    HANDLE processHandle = nullptr;  // Updated to HANDLE instead of void*
    bool debug = false;

public:
    // Default constructor (process handle not initialized)
    Memory() noexcept = default;

    // Destructor: Closes the process handle
    ~Memory()
    {
        if (processHandle) {
            ::CloseHandle(processHandle);
        }
    }

    // Static method to return a HANDLE directly
    static HANDLE GetHandle(const std::string_view processName, bool debugMode = false) noexcept
    {
        PROCESSENTRY32 entry = {};
        entry.dwSize = sizeof(entry);

        const auto snapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapShot == INVALID_HANDLE_VALUE) {
            if (debugMode) std::cerr << "Failed to create process snapshot.\n";
            return nullptr;
        }

        HANDLE processHandle = nullptr;
        while (::Process32Next(snapShot, &entry)) {
            if (processName == entry.szExeFile) {
                DWORD processId = entry.th32ProcessID;
                processHandle = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
                if (!processHandle && debugMode) {
                    std::cerr << "Failed to open process handle.\n";
                }
                break;
            }
        }

        ::CloseHandle(snapShot);
        return processHandle;
    }

    // Function to set the internal handle after calling GetHandle()
    void SetHandle(HANDLE handle) noexcept
    {
        processHandle = handle;
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
