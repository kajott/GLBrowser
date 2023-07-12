// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#include <string>
#include <functional>

struct FileAssociationRegistryItem {
    const char* displayName;
    const char* executableName;
    bool        allowExec;
    const char* extensions;
};

struct FileAssociation : public FileAssociationRegistryItem {
    std::string executablePath;
    int index;  // guaranteed to be >0 for a valid association
};

//! initialize file association logic
void FileAssocInit(const char* argv0=nullptr);

//! look up a file assoctiation by extension code
//! \param callback  function to enumerate the associated programs with;
//!                  returns true to continue enumeration, or false to stop
//! \returns number of programs associated with this extension
int FileAssocLookup(uint32_t extCode, std::function<bool(const FileAssociation& assoc)> callback=nullptr);

//! get a file association by index
const FileAssociation& GetFileAssoc(int index);
