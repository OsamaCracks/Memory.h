#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <winternl.h>
#include <string>
#include <iostream>
#include <cstdint>
#include <memory>
#include <optional>

//Yeah im 5 days late lol
//Set to c++ 20
//Make multi-byte instead of unicode
//Set to release

#define WINDOWS_IGNORE_PACKING_MISMATCH

// Dynamic function pointers for NtReadVirtualMemory and NtWriteVirtualMemory
typedef NTSTATUS(NTAPI* NtReadVirtualMemory_t)(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    ULONG NumberOfBytesToRead,
    PULONG NumberOfBytesRead OPTIONAL
    );

typedef NTSTATUS(NTAPI* NtWriteVirtualMemory_t)(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    ULONG NumberOfBytesToWrite,
    PULONG NumberOfBytesWritten OPTIONAL
    );

class Memory {
private:
    std::uintptr_t processId = 0; // Stores the process ID of the target process
    HANDLE processHandle = nullptr; // Handle to the target process
    bool debug = false; // Debug flag to enable or disable debug messages

    // Pointers to dynamically loaded NtReadVirtualMemory and NtWriteVirtualMemory functions
    NtReadVirtualMemory_t pNtReadVirtualMemory = nullptr;
    NtWriteVirtualMemory_t pNtWriteVirtualMemory = nullptr;

    // Dynamically load NtReadVirtualMemory and NtWriteVirtualMemory from ntdll.dll
    bool LoadNtFunctions() noexcept {
        HMODULE ntdll = GetModuleHandle("ntdll.dll");
        if (!ntdll) {
            if (debug) {
                std::cerr << "[ERROR] Failed to load ntdll.dll" << std::endl;
            }
            return false;
        }

        // Load NtReadVirtualMemory
        pNtReadVirtualMemory = reinterpret_cast<NtReadVirtualMemory_t>(
            GetProcAddress(ntdll, "NtReadVirtualMemory"));
        if (!pNtReadVirtualMemory && debug) {
            std::cerr << "[ERROR] Failed to find NtReadVirtualMemory" << std::endl;
        }

        // Load NtWriteVirtualMemory
        pNtWriteVirtualMemory = reinterpret_cast<NtWriteVirtualMemory_t>(
            GetProcAddress(ntdll, "NtWriteVirtualMemory"));
        if (!pNtWriteVirtualMemory && debug) {
            std::cerr << "[ERROR] Failed to find NtWriteVirtualMemory" << std::endl;
        }

        return pNtReadVirtualMemory && pNtWriteVirtualMemory;
    }

    // Helper function to handle errors and print debug messages
    void HandleError(const char* operation, NTSTATUS status) const noexcept {
        if (status < 0) { // STATUS_SUCCESS = 0, negative values indicate failure
            if (debug) {
                std::cerr << "[ERROR] " << operation << " failed with NTSTATUS: 0x"
                    << std::hex << status << std::dec << std::endl;
            }
        }
    }

public:
    // Constructor to initialize the Memory class with a target process name and optional debug mode
    bool GetHandle(std::string_view processName, bool debugMode = false) noexcept {
        debug = debugMode;
        PROCESSENTRY32 entry = {}; // Structure to hold process information
        entry.dwSize = sizeof(PROCESSENTRY32);

        HANDLE snapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapShot == INVALID_HANDLE_VALUE) {
            if (debug) {
                std::cerr << "[ERROR] Failed to create process snapshot." << std::endl;
            }
            return false;
        }

        while (Process32Next(snapShot, &entry)) {
#ifdef UNICODE
            if (processName == std::wstring(entry.szExeFile))
#else
            if (processName == std::string(entry.szExeFile))
#endif
            {
                processId = entry.th32ProcessID;
                processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, static_cast<DWORD>(processId));
                if (!processHandle) {
                    if (debug) {
                        std::cerr << "[ERROR] Failed to open process: " << processName << std::endl;
                    }
                    CloseHandle(snapShot);
                    return false;
                }
                break;
            }
        }

        CloseHandle(snapShot);

        // Ensure NtReadVirtualMemory and NtWriteVirtualMemory are loaded
        if (!LoadNtFunctions()) {
            return false;
        }

        return processHandle != nullptr;
    }

    // Destructor to clean up and release the process handle
    ~Memory() {
        if (processHandle) {
            CloseHandle(processHandle);
        }
    }

    // Retrieve the base address of a module in the target process by its name
    std::uintptr_t GetModuleAddress(std::string_view moduleName) const noexcept {
        MODULEENTRY32 entry = {};
        entry.dwSize = sizeof(MODULEENTRY32);

        HANDLE snapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, static_cast<DWORD>(processId));
        if (snapShot == INVALID_HANDLE_VALUE) {
            if (debug) {
                std::cerr << "[ERROR] Failed to create module snapshot." << std::endl;
            }
            return 0;
        }

        std::uintptr_t result = 0;
        while (Module32Next(snapShot, &entry)) {
#ifdef UNICODE
            if (moduleName == std::wstring(entry.szModule))
#else
            if (moduleName == std::string(entry.szModule))
#endif
            {
                result = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
                break;
            }
        }

        CloseHandle(snapShot);
        return result;
    }

    // Read memory from the target process using NtReadVirtualMemory
    template <typename T>
    std::optional<T> Read(const std::uintptr_t address) const noexcept {
        if (!pNtReadVirtualMemory) {
            if (debug) {
                std::cerr << "[ERROR] NtReadVirtualMemory is not loaded." << std::endl;
            }
            return std::nullopt;
        }

        T value = {};
        NTSTATUS status = pNtReadVirtualMemory(
            processHandle,
            reinterpret_cast<PVOID>(address),
            &value,
            sizeof(T),
            nullptr
        );

        if (status < 0) {
            HandleError("NtReadVirtualMemory", status);
            return std::nullopt;
        }
        else if (debug) {
            std::cout << "[DEBUG] Read memory at address 0x"
                << std::hex << address << ", value: " << value << std::dec << std::endl;
        }

        return value;
    }

    // Write memory to the target process using NtWriteVirtualMemory
    template <typename T>
    bool Write(const std::uintptr_t address, const T& value) const noexcept {
        if (!pNtWriteVirtualMemory) {
            if (debug) {
                std::cerr << "[ERROR] NtWriteVirtualMemory is not loaded." << std::endl;
            }
            return false;
        }

        NTSTATUS status = pNtWriteVirtualMemory(
            processHandle,
            reinterpret_cast<PVOID>(address),
            const_cast<T*>(&value),
            sizeof(T),
            nullptr
        );

        if (status < 0) {
            HandleError("NtWriteVirtualMemory", status);
            return false;
        }
        else if (debug) {
            std::cout << "[DEBUG] Wrote memory at address 0x"
                << std::hex << address << ", value: " << value << std::dec << std::endl;
        }

        return true;
    }
};
