export QUARTUS_ROOTDIR=/opt/intelFPGA/18.0/quartus
export ALTERAOCLSDKROOT=/opt/intelFPGA/18.0/hld
export PATH=$PATH:$QUARTUS_ROOTDIR/bin:/opt/intelFPGA/18.0/embedded/ds-5/bin:/opt/intelFPGA/18.0/embedded/ds-5/sw/gcc/bin:$ALTERAOCLSDKROOT/bin:$ALTERAOCLSDKROOT/linux64/bin:
export LD_LIBRARY_PATH=$ALTERAOCLSDKROOT/linux64/lib
export AOCL_BOARD_PACKAGE_ROOT=$ALTERAOCLSDKROOT/board/terasic/de10_nano
#export AOCL_BOARD_PACKAGE_ROOT=$ALTERAOCLSDKROOT/board/c5soc
export QUARTUS_64BIT=1
export LM_LICENSE_FILE=/home/ogura/Licese.dat

#2017.11.04 Reblanded ALTERAOCLSDKROOT to
export INTELFPGAOCLSDKROOT=$ALTERAOCLSDKROOT
export LD_LIBRARY_PATH=$INTELFPGAOCLSDKROOT/host/linux64/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$AOCL_BOARD_PACKAGE_ROOT/linux64/lib:$LD_LIBRARY_PATH
export QUARTUS_ROOTDIR_OVERRIDE=$QUARTUS_ROOTDIR
export TMPDIR=/home/ogura/tmpdir

#2017.11.04 Reblanded CL_CONTEXT_EMULATOR_DEVICE_ALTERA to
#export CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=0
