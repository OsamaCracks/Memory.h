#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <string>
#include <iostream>

// Global handle for process (assumed declared somewhere)
extern HANDLE gHandle;

// Helper function to convert wide string (WCHAR) to std::string
std::string WideStringToString(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}

// Get handle to process by process name (without .exe extension)
HANDLE GetHandle(const std::string& processName) noexcept
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
        std::string exeFile = WideStringToString(entry.szExeFile);  // Convert wide char to std::string
        if (exeFile.substr(0, exeFile.size() - 4) == processName)  // Compare without ".exe"
        {
            DWORD processId = entry.th32ProcessID;
            processHandle = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

            if (processHandle == nullptr) {
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
std::uintptr_t GetModuleAddress(HANDLE processHandle, DWORD processId, const std::string& moduleName) noexcept
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
        std::string modFile = WideStringToString(entry.szModule);  // Convert wide char to std::string
        if (modFile == moduleName)  // Direct comparison using std::string
        {
            moduleBase = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
            std::cout << "[+] Module '" << moduleName << "' found at address: " << std::hex << moduleBase << std::endl;
            break;
        }
    }

    ::CloseHandle(snapShot);
    return moduleBase;
}

// Template function to read process memory
template <typename T>
T Read(std::uintptr_t address) noexcept
{
    T value;
    if (!::ReadProcessMemory(gHandle, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), nullptr)) {
        std::cerr << "[-] Failed to read memory at address: " << std::hex << address << std::endl;
    }
    return value;
}

// Template function to write process memory
template <typename T>
bool Write(std::uintptr_t address, T value) noexcept
{
    if (!::WriteProcessMemory(gHandle, reinterpret_cast<LPVOID>(address), &value, sizeof(T), nullptr)) {
        std::cerr << "[-] Failed to write memory at address: " << std::hex << address << std::endl;
        return false;
    }
    return true;
}
