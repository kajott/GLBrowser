// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <cstdio>

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#include "sysutil.h"

#include "file_assoc.h"

///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
    #define EXE(x) (x ".exe")
#else
    #define EXE(x) x
#endif

static const FileAssociationRegistryItem fileAssocRegistry[] = {
    { "GLISS",     EXE("gliss"),     "/ jpg jpeg jpe" },
    { "XnView",    EXE("xnview"),    "/ jpg jpeg jpe jfif png bmp tif tiff tga pcx gif" },

    { "MPV",       EXE("mpv"),       "mp4 mov mkv webm mts m2ts m2t m2p mpg ogv wmv asf flv avi" },
    { "MPC-HC",    EXE("mpc-hc64"),  "mp4 mov mkv webm mts m2ts m2t m2p mpg ogv wmv asf flv avi" },
    { "VLC",       EXE("vlc"),       "mp4 mov mkv webm mts m2ts m2t m2p mpg ogv wmv asf flv avi" },

    { "MuPDF",     EXE("mupdf"),     "pdf" },

    { "Vivaldi",   EXE("vivaldi"),   "/ htm html" },
    { "Chrome",    EXE("chrome"),    "/ htm html" },
    { "Chromium",  EXE("chromium"),  "/ htm html" },
    { "Firefox",   EXE("firefox"),   "/ htm html" },

    { "Notepad++", EXE("notepad++"), "txt md c cc cpp cxx h hh hpp hxx rs java cs kt js htm html py pl php rb sh pas dpr inc asm diz nfo json xml yaml ini conf" },
    { "GEdit",     EXE("gedit"),     "txt md c cc cpp cxx h hh hpp hxx rs java cs kt js htm html py pl php rb sh pas dpr inc asm diz nfo json xml yaml ini conf" },
    { "VS Code",   EXE("code"),    "/ txt md c cc cpp cxx h hh hpp hxx rs java cs kt js htm html py pl php rb sh pas dpr inc asm diz nfo json xml yaml ini conf" },

    // stuff that executes scripts is put last, so it never becomes the default
    { "Python",    EXE("py"),        "py" },
    { "Python",    EXE("python"),    "py" },
    { "Perl",      EXE("perl"),      "pl" },
    { "bash",      EXE("bash"),      "sh" },
    { nullptr, nullptr, nullptr },
};

///////////////////////////////////////////////////////////////////////////////

static std::vector<FileAssociation> assocList;
static std::unordered_map<uint32_t, std::vector<int>> extMap;

void FileAssocInit(const char* argv0) {
    assocList.clear();
    extMap.clear();
    if (argv0) {
        FindProgramInit(PathDirName(argv0));
    }

    FileAssociation assoc;
    assoc.displayName = assoc.executableName = assoc.extensions = nullptr;
    assoc.index = 0;
    assocList.push_back(assoc);

    for (const auto* item = fileAssocRegistry;  item->displayName && item->executableName && item->extensions;  ++item) {
        assoc.executablePath = FindProgram(item->executableName);
        if (assoc.executablePath.empty()) { continue; }
        assoc.displayName    = item->displayName;
        assoc.executableName = item->executableName;
        assoc.extensions     = item->extensions;
        assoc.index          = int(assocList.size());
        assocList.push_back(assoc);

        #ifndef NDEBUG
            printf("found program: %-10s -> %s\n", item->displayName, assoc.executablePath.c_str());
        #endif

        // parse extension list
        const char* pos = item->extensions;
        uint32_t code = 0u;
        char c;
        do {
            c = *pos++;
            if (!c || (c == ' ')) {
                if (code) { extMap[code].push_back(assoc.index); }
                code = 0u;
            } else if (code < (1u << 24)) {
                code = (code << 8) | uint8_t(c);
            }
        } while (c);
    }

    #if 0  // DEBUG: dump file associations
        for (int i = 1;  i < int(assocList.size());  ++i) {
            printf("[%02d] %-10s -> %s\n", i, assocList[i].displayName, assocList[i].executablePath.c_str());
        }
        for (auto it : extMap) {
            printf("%c%c%c%c", (it.first & 0xFF000000u) ? char(it.first >> 24) : '_',
                               (it.first & 0x00FF0000u) ? char(it.first >> 16) : '_',
                               (it.first & 0x0000FF00u) ? char(it.first >>  8) : '_',
                               (it.first & 0x000000FFu) ? char(it.first >>  0) : '_');
            for (auto index : it.second) {
                printf(" -> %s", assocList[index].displayName);
            }
            printf("\n");
        }
    #endif
}

int FileAssocLookup(uint32_t extCode, std::function<bool(const FileAssociation& assoc)> callback) {
    auto res = extMap.find(extCode);
    if (res == extMap.end()) { return 0; }
    if (callback) {
        for (int index : res->second) {
            if (!callback(assocList[index])) { break; }
        }
    }
    return int(res->second.size());
}

const FileAssociation& GetFileAssoc(int index) {
    return assocList[((index > 0) && (index < int(assocList.size()))) ? index : 0];
}
