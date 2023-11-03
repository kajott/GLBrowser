#!/bin/bash
binary=glbrowser

# build, if not already done so
if [ ! -x $binary ] ; then
    echo "***** no binary found, building ..."
    ( set -ex ; cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Release ) || exit $?
    ( set -ex ; cmake --build build ) || exit $?
fi

# set candidate paths
path_user="$HOME/.local/bin/$binary"
path_system="/usr/local/bin/$binary"
path_local="$(realpath "$binary")"

# check for existing installation of the same binary
install_path=""
hash="$(sha1sum "$binary" | cut -d' ' -f1)"
if [ -z "$install_path" ] ; then
    if echo "$hash $path_user" | sha1sum --check --quiet >&/dev/null ; then
        install_path="$path_user"
    fi
fi
if [ -z "$install_path" ] ; then
    if echo "$hash $path_system" | sha1sum --check --quiet >&/dev/null ; then
        install_path="$path_system"
    fi
fi

# ask where to install to (unless a binary has already been found)
if [ -n "$install_path" ] ; then
    echo "Binary is already installed at '$install_path'."
else
    echo "No installed version of $binary found at the usual locations."
    echo
    echo "Select install location:"
    echo -n "  1. $path_user"
    if [ -x "$path_user" ]                         ; then echo " [already exists]"
    elif [[ ":$PATH:" == *":$HOME/.local/bin:"* ]] ; then echo " [recommended]"
    else                                                  echo " [not in PATH]"
    fi
    echo -n "  2. $path_system"
    if [ -x "$path_system" ] ; then echo " [already exists; requires sudo privileges]"
    else                            echo " [requires sudo privileges]"
    fi
    echo "* 3. $path_local [don't install]"
    echo "Your choice (1/2/3, or custom *absolute* path, *including* the file name)?"
    echo -n "=> " ; read install_path
    [ "$install_path" == "1" ] && install_path="$path_user"
    [ "$install_path" == "2" ] && install_path="$path_system"
    [ "$install_path" == "3" ] && install_path=""
    if [ -n "$install_path" ] ; then
        if [[ "$install_path" != /* ]] ; then
            echo "ERROR: custom path is not absolute, aborting."
            exit 2
        fi
    fi

    # actually install
    if [ -n "$install_path" ] ; then
        echo "Installing to $install_path ..."
        install_dir="$(dirname "$install_path")"
        if [ ! -d "$install_dir" ] ; then
            ( set -ex ; mkdir -p "$install_dir" ) || exit $?
        fi
        ( set -ex ;      install -m 755 "$binary" "$install_path" ) || \
        ( set -ex ; sudo install -m 755 "$binary" "$install_path" ) || exit 1
    fi
fi
