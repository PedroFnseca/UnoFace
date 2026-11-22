#include "unoface/platform/win32_system.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#endif

#include <cstdio>

namespace unoface {
namespace platform {

#ifdef _WIN32

std::string local_timestamp() {
    SYSTEMTIME t;
    GetLocalTime(&t);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u.%03u",
                  t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    return buf;
}

std::string file_timestamp() {
    SYSTEMTIME t;
    GetLocalTime(&t);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04u%02u%02u-%02u%02u%02u",
                  t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    return buf;
}

std::string executable_directory() {
    char path[MAX_PATH] = {0};
    const DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string full(path, length);
    const size_t slash = full.find_last_of("\\/");
    return slash == std::string::npos ? std::string(".") : full.substr(0, slash);
}

bool ensure_directory(const std::string& path) {
    if (CreateDirectoryA(path.c_str(), nullptr)) {
        return true;
    }
    return GetLastError() == ERROR_ALREADY_EXISTS;
}

void open_in_explorer(const std::string& path) {
    ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

#else

std::string local_timestamp() { return std::string(); }
std::string file_timestamp() { return std::string(); }
std::string executable_directory() { return "."; }
bool ensure_directory(const std::string&) { return false; }
void open_in_explorer(const std::string&) {}

#endif

} // namespace platform
} // namespace unoface
