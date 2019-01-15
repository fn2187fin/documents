#! /bin/sh
#
# Copyright (C) 2013 ARM Ltd. All rights reserved.
#
# ds-deps.sh [--no-install]
# 
# Check for and install the DS-5 system dependencies.
#
#   --no-install  Only check whether the dependencies are installed, do not install
#
#

check_for_package () 
{
    printf "Checking for $1... "
    if `yum -q info $1 | grep "Installed" > /dev/null`; then
        printf "installed\n"
    else
        printf "not installed\n"
        install_package $1
    fi
}

install_package ()
{
    if $install; then
        printf "Installing $1... "
        if `yum -q -y install $1`; then
            printf "done\n"
        fi
    fi
}

install=true

if [ $# -gt 0 ] ; then
    if [ $1 = "--no-install" ] ; then
        install=false
    else
        echo "Usage: ds-deps.sh [--no-install]"
        echo "Check for and install the DS-5 system dependencies."
        echo ""
        echo "  --no-install  Only check whether the dependencies are installed, do not install"
        exit
    fi
fi

if [ `whoami` != root ] ; then
    echo "Error: Dependency management requires root privileges"
    exit 1
fi

# 64-bit dependencies
check_for_package 'glibc' 
check_for_package 'gtk2' 
check_for_package 'libstdc++'
check_for_package 'alsa-lib'
check_for_package 'atk'
check_for_package 'cairo'
check_for_package 'fontconfig'
check_for_package 'freetype'
check_for_package 'libX11'
check_for_package 'libXext'
check_for_package 'libXi'
check_for_package 'libXrender'
check_for_package 'libXt'
check_for_package 'libXtst'
check_for_package 'webkitgtk' 

# 32-bit dependencies
check_for_package 'glibc.i686' 
check_for_package 'fontconfig.i686'
check_for_package 'freetype.i686'
check_for_package 'libICE.i686'
check_for_package 'ncurses-libs.i686'
check_for_package 'libSM.i686'
check_for_package 'libstdc++.i686'
check_for_package 'libusb.i686'
check_for_package 'libX11.i686'
check_for_package 'libXcursor.i686'
check_for_package 'libXext.i686'
check_for_package 'libXft.i686'
check_for_package 'libXmu.i686'
check_for_package 'libXrandr.i686'
check_for_package 'libXrender.i686'
check_for_package 'zlib.i686'


printf "Checking for libGL.so.1 (32 bit)... "
if `ldconfig -p | grep "libGL.so.1" | grep -v "x86-64" > /dev/null`; then
	printf "found\n"
else
	printf "not found\n"
	install_package 'mesa-libGL.i686'
fi
