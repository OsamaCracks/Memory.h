#pragma once
#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>  // Required for CreateToolhelp32Snapshot
#include <cstdint>
#include <string_view>

// Read process memory
template <typename T>
constexpr T read(HANDLE processHandle, const std::uintptr_t& address) noexcept
{
    T value = { };
    ::ReadProcessMemory(processHandle, reinterpret_cast<const void*>(address), &value, sizeof(T), NULL);
    return value;
}

// Write process memory
template <typename T>
constexpr void write(HANDLE processHandle, const std::uintptr_t& address, const T& value) noexcept
{
    ::WriteProcessMemory(processHandle, reinterpret_cast<void*>(address), &value, sizeof(T), NULL);
}

// Get handle to process by window name
HANDLE GetHandle(const char* windowName)
{
    HWND hWnd = FindWindowA(NULL, windowName);
    if (hWnd == NULL) {
        std::cerr << "Could not find window: " << windowName << std::endl;
        return NULL;
    }

    DWORD processID;
    GetWindowThreadProcessId(hWnd, &processID);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (hProcess == NULL) {
        std::cerr << "Could not open process with ID: " << processID << std::endl;
    }

    return hProcess;
}
