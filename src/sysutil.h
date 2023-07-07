#pragma once

#include <string>
#include <functional>

#ifdef _WIN32
    constexpr char pathSep = '\\';
    constexpr char searchPathSep = ';';
    constexpr const char* rootDir = "";
#else
    constexpr char pathSep = '/';
    constexpr char searchPathSep = ':';
    constexpr const char* rootDir = "/";
#endif

inline constexpr bool my_isalpha(char c)
    { return ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')); }

bool ispathsep(char c);

std::string PathJoin(const char* a, const char* b);
inline std::string PathJoin(const std::string& a, const std::string& b)
    { return PathJoin(a.c_str(), b.c_str()); }

const char* PathBaseNamePtr(const char* path);
inline std::string PathBaseName(const char* path)        { return PathBaseNamePtr(path); }
inline std::string PathBaseName(const std::string& path) { return PathBaseNamePtr(path.c_str()); }

std::string PathDirName(const char* path);
inline std::string PathDirName(const std::string& path)  { return PathDirName(path.c_str()); }

std::string GetCurrentDir();

bool PathExists(const char* path);
inline bool PathExists(const std::string& path)  { return PathExists(path.c_str()); }

bool IsDirectory(const char* path);
inline bool IsDirectory(const std::string& path) { return IsDirectory(path.c_str()); }

bool IsFile(const char* path);
inline bool IsFile(const std::string& path)      { return IsFile(path.c_str()); }

bool IsRoot(const char* path);
inline bool IsRoot(const std::string& path)      { return IsRoot(path.c_str()); }

bool ScanDirectory(const char* path, std::function<void(const char* name, bool isdir)> callback);
