#define _CRT_SECURE_NO_WARNINGS

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
#include <cstring>

#include <string>
#include <vector>
#include <functional>

#include "sysutil.h"

///////////////////////////////////////////////////////////////////////////////

constexpr int currentDirMaxLen = 1024;

uint32_t extractExtCode(const char* path) {
    if (!path) { return 0u; }
    const char* ext = nullptr;
    while (*path) {
        if (ispathsep(*path))  { ext = nullptr; }
        else if (*path == '.') { ext = &path[1]; }
        ++path;
    }
    if (!ext) { return 0u; }
    uint32_t code = 0u;
    do {
        char c = *ext++;
        if (!c) { break; }
        if ((c >= 'A') && (c <= 'Z')) { c += 'a' - 'A'; }
        code = (code << 8) | uint8_t(*ext);
    } while (code < (1u << 24));
    return code;
}

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

static std::vector<std::string> searchDirs;

void FindExecutableInit(const char* additionalDir) {
    searchDirs.emplace_back("");
    if (additionalDir && additionalDir[0]) {
        searchDirs.emplace_back(additionalDir);
    }
    const char *pathVar = getenv("PATH");
    if (pathVar) {
        int start = 0;
        while (pathVar[start]) {
            int end = start;
            while (pathVar[end] && (pathVar[end] != searchPathSep)) { ++end; }
            if (end > start) {
                searchDirs.emplace_back(&pathVar[start], end - start);
            }
            start = pathVar[end] ? (end + 1) : end;
        }
    }
    //for (const auto& dir : searchDirs) { printf("SD: %s\n", dir.c_str()); }
}

std::string FindExecutable(const char* name) {
    if (searchDirs.empty()) {
        FindExecutableInit();
    }
    for (const auto& searchDir : searchDirs) {
        std::string fullPath = PathJoin(searchDir, name);
        if (IsExecutable(fullPath)) { return fullPath; }
    }

#ifdef _WIN32
    // look for in the registry, too
    HKEY hKey;
    std::string keyName("Applications\\");
    keyName.append(name);
    keyName.append("\\shell\\open\\command");
    if (RegOpenKeyExA(HKEY_CLASSES_ROOT, keyName.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char cmd[currentDirMaxLen];
        DWORD typeCode, size = DWORD(sizeof(cmd));
        cmd[0] = '\0';
        if ((RegQueryValueExA(hKey, nullptr, nullptr, &typeCode, LPBYTE(cmd), &size) == ERROR_SUCCESS)
        &&  (typeCode == REG_SZ)) {
            int start = 0, end;
            while (cmd[start] && (cmd[start] == ' ')) { ++start; }
            if (cmd[start] == '"') {
                end = ++start;
                while (cmd[end] && (cmd[end] != '"')) { ++end; }
            } else {
                end = start;
                while (cmd[end] && (cmd[end] != ' ')) { ++end; }
            }
            std::string fullPath(&cmd[start], end-start);
            if (IsExecutable(fullPath)) { return fullPath; }
        }
        RegCloseKey(hKey);
    }
#endif
    return "";  // not found
}

#ifdef _WIN32 /////////////////////////////////////////////////////////////////

bool ispathsep(char c) { return (c == '\\') || (c == '/'); }

static bool IsExeFile(const char* path) {
    if (!path) { return false; }
    int len = int(strlen(path));
    return (len > 4) &&  (path[len-4] == '.')
                     && ((path[len-3] == 'e') || (path[len-3] == 'E'))
                     && ((path[len-2] == 'x') || (path[len-2] == 'X'))
                     && ((path[len-1] == 'e') || (path[len-1] == 'E'));
}

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

bool IsExecutable(const char* path) {
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY) && IsExeFile(path);
}

bool ScanDirectory(const char* path, std::function<void(const char*, bool, bool)> callback) {
    if (!path || !path[0]) {
        // special case: empty path -> generate drive list
        DWORD mask = GetLogicalDrives();
        if (!mask) { return false; }
        char drive[] = "A:";
        for (int i = 26;  i;  --i) {
            if (mask & 1u) { callback(drive, true, false); }
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
            int fnlen = int(strlen(item.cFileName));
            bool isdir = !!(item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
            callback(item.cFileName, isdir, !isdir && IsExeFile(item.cFileName));
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
    return (stat(path, &st) == 0) && !S_ISDIR(st.st_mode);
}

bool IsDirectory(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0) && !!S_ISDIR(st.st_mode);
}

bool IsExecutable(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0) && !S_ISDIR(st.st_mode) && ((st.st_mode & 0111) != 0);
}

bool ScanDirectory(const char* path, std::function<void(const char*, bool, bool)> callback) {
    DIR *dir = opendir(path);
    if (!dir) { return false; }
    struct dirent* item;
    struct stat st;
    while ((item = readdir(dir))) {
        if (!item->d_name[0] || (item->d_name[0] == '.'))
            { continue; }  // ignore hidden items
        std::string itemPath = PathJoin(path, item->d_name);
        if (stat(itemPath.c_str(), &st) == 0) {
            bool isdir = !!S_ISDIR(st.st_mode);
            callback(item->d_name, isdir, !isdir && ((st.st_mode & 0111) != 0));
        }
    }
    closedir(dir);
    return true;
}

#endif // POSIX ///////////////////////////////////////////////////////////////
