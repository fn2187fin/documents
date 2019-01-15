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
    if `dpkg -s $1 2> /dev/null | grep "Status: install ok installed" > /dev/null`; then
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
        if `apt-get -qq -y install $1 > /dev/null`; then
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
check_for_package 'libc6' 
check_for_package 'libstdc++6'
check_for_package 'libgtk2.0-0' 
check_for_package 'libasound2'
check_for_package 'libatk1.0-0'
check_for_package 'libcairo2'
check_for_package 'libfontconfig1'
check_for_package 'libfreetype6'
check_for_package 'libglib2.0-0'
check_for_package 'libx11-6'
check_for_package 'libxext6'
check_for_package 'libxi6'
check_for_package 'libxrender1'
check_for_package 'libxt6'
check_for_package 'libxtst6' 
check_for_package 'libwebkitgtk-1.0-0'

# 32-bit dependencies
check_for_package 'libc6:i386'
check_for_package 'libfontconfig1:i386'
check_for_package 'libfreetype6:i386'
check_for_package 'libice6:i386'
check_for_package 'libncurses5:i386'
check_for_package 'libsm6:i386'
check_for_package 'libstdc++6:i386'
check_for_package 'libusb-0.1-4:i386'
check_for_package 'libx11-6:i386'
check_for_package 'libxcursor1:i386'
check_for_package 'libxext6:i386'
check_for_package 'libxft2:i386'
check_for_package 'libxmu6:i386'
check_for_package 'libxrandr2:i386'
check_for_package 'libxrender1:i386'
check_for_package 'zlib1g:i386'

printf "Checking for libGL.so.1 (32 bit)... "
if `ldconfig -p | grep "libGL.so.1" | grep -v "x86-64" > /dev/null`; then
	printf "found\n"
else
	printf "not found\n"
	install_package 'libgl1-mesa-glx:i386'
fi
