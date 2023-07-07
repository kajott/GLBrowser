#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <dirent.h>
#endif

#include <cstdio>

#include <string>
#include <functional>

#include "sysutil.h"

///////////////////////////////////////////////////////////////////////////////

constexpr int currentDirMaxLen = 1024;

std::string PathJoin(const char* a, const char* b) {
    std::string res;
    if      (!a || !a[0]) { res = b; }
    else if (!b || !b[0]) { res = a; }
    else {
        res = a;
        if (!ispathsep(res[res.size() - 1])) { res.append(1, pathSep); }
        res.append(b);
    }
    #ifdef _WIN32
        if ((res.size() == 2u) && my_isalpha(res[0]) && (res[1] == ':'))
            { res.append(1, pathSep); }  // turn 'X:' into 'X:\'
    #endif
    return res;
}

const char* PathBaseNamePtr(const char* path) {
    if (!path) { return ""; }
    const char* res = path;
    for (;  *path;  ++path) {
        if (ispathsep(*path)) { res = &path[1]; }
    }
    return res;
}

std::string PathDirName(const char* path) {
    // special handling for root directories
    #ifdef _WIN32
        if (path && my_isalpha(path[0]) && (path[1] == ':') && (!path[2] || (ispathsep(path[2]) && !path[3])))
            { return ""; }  // turn 'X:' and 'X:\' into the empty string (drive selector)
    #endif

    // generic case
    std::string res(path);
    int pos = int(res.size()) - 1;
    while ((pos > 0) && !ispathsep(res[pos])) { --pos; }
    if (pos >= 0) { res.resize(pos); }

    // more special handling for root directories
    #ifdef _WIN32
        if ((pos == 2) && my_isalpha(res[0]) && (res[1] == ':'))
            { res.append(1, pathSep); }  // turn 'X:' into 'X:\'
        if ((pos == 1) && path && ispathsep(path[0]) && ispathsep(path[1]))
            { return ""; }  // UNC root
    #else
        if ((pos < 1) && path && ispathsep(path[0]))
            { return "/"; }  // root directory
    #endif
    return res;
}

bool IsRoot(const char* path) {
    return !path || !path[0] || (ispathsep(path[0]) && !path[1]);
}

#ifdef _WIN32 /////////////////////////////////////////////////////////////////

bool ispathsep(char c) { return (c == '\\') || (c == '/'); }

std::string GetCurrentDir() {
    char buf[currentDirMaxLen];
    return GetCurrentDirectory(currentDirMaxLen, buf) ? buf : "";
}

bool PathExists(const char* path) {
    return (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES);
}

bool IsFile(const char* path) {
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool IsDirectory(const char* path) {
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && !!(attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool ScanDirectory(const char* path, std::function<void(const char*, bool)> callback) {
    if (!path || !path[0]) {
        // special case: empty path -> generate drive list
        DWORD mask = GetLogicalDrives();
        if (!mask) { return false; }
        char drive[] = "A:";
        for (int i = 26;  i;  --i) {
            if (mask & 1u) { callback(drive, true); }
            mask >>= 1;
            drive[0]++;
        }
        return true;
    }
    std::string wildcard = PathJoin(path, "*");
    WIN32_FIND_DATAA item;
    HANDLE dir = FindFirstFileA(wildcard.c_str(), &item);
    if (dir == INVALID_HANDLE_VALUE) { return false; }
    do {
        if (item.cFileName[0] && (item.cFileName[0] != '.') && !(item.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))) {
            callback(item.cFileName, !!(item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
        }
    } while (FindNextFileA(dir, &item));
    FindClose(dir);
    return true;
}

#else // POSIX ////////////////////////////////////////////////////////////////

bool ispathsep(char c) { return (c == '/'); }

std::string GetCurrentDir() {
    char buf[currentDirMaxLen];
    return getcwd(buf, currentDirMaxLen);
}

bool PathExists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

bool IsFile(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0) && !!S_ISREG(st.st_mode);
}

bool IsDirectory(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0) && !!S_ISDIR(st.st_mode);
}

bool ScanDirectory(const char* path, std::function<void(const char*, bool)> callback) {
    DIR *dir = opendir(path);
    if (!dir) { return false; }
    struct dirent* item;
    struct stat st;
    while ((item = readdir(dir))) {
        if (!item->d_name[0] || (item->d_name[0] == '.'))
            { continue; }  // ignore hidden items
        std::string itemPath = PathJoin(path, item->d_name);
        if (stat(itemPath.c_str(), &st) == 0) {
            callback(item->d_name, !!S_ISDIR(st.st_mode));
        }
    }
    closedir(dir);
    return true;
}

#endif // POSIX ///////////////////////////////////////////////////////////////
