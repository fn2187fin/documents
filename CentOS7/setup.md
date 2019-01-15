# CentOS7 Setup

### とにかく安全に入れ替える！方法は？
PC-linuxは予期せずぶっとぶ;-<。 telnetなどは生きていても、どうにもGUI復旧せず 入れ替え  
- Windows Cドライブを引き抜いて、安全策。 SATAにインストール先となるHDDのみ挿しておく、これでWindowsは絶対安全
- rufusをCentos7インストーラとし、BIOSからUSBブートする
日本語キーボード　レイアウトへ変更できず　やり直し
インストール時に間違えると、変更ができない?
日本語キーボードレイアウトに日本語入力と英字入力の2つがあるらしい
Saver with GUIではなく、GNOME Desktopにしてみた

### ctrl and shift key swap
[CapsLockキーとCtrlキーを入れ替える](https://linuxfan.info/capslock-ctrl)  
gsettings set org.gnome.desktop.input-sources xkb-options "['ctrl:swapcaps']"  
tweekでfocusをMouseへ変更  

### dotファイル
参考dotファイル
- [.bashrc](./dot.bashrc)
- [.vimrc](./dot.vimrc)
- [.xinitrc](./dot.xinitrc)
- [.gitconfig](./dot.gitconfig)
- [.screenrc](./dot.screenrc)
- [.netrc](./dot.netrc)

### ntfs usbドライブ ###
[ここ](https://ameblo.jp/sweet-lorraine/entry-12121866539.html)  

```
$ yum -y install epel-release  
$ yum -y --enablerepo=epel install fuse dkms dkms-fuse fuse-ntfs-3g  
$ parted -l
...
モデル: TOSHIBA External USB 3.0 (scsi)
ディスク /dev/sdc: 1000GB
セクタサイズ (論理/物理): 512B/512B
パーティションテーブル: msdos
ディスクフラグ: 
番号  開始    終了    サイズ  タイプ   ファイルシステム  フラグ
 1    1049kB  1000GB  1000GB  primary  ntfs              boot
...  
```

/dev/sdc1 がUSB HDDなので、  
$ mount /dev/sdc1 /mnt  
ただし、USB HDDはファイル権限などの変更が効かないので注意

### OpenCL Environment ###
Altera FPGA開発
```
$ SoCEDSSetup-18.0.0.614-linux.run  
$ /opt/intelFPGA/18.0/embedded/host_tools/altera/ds5_link/ds5_link  //linkが足りないなぁ
$ cd /opt/intelFPGA/18.0/embedded                                   //linkが足りないなぁ
$ ln -s /usr/local/DS-5_v5.28.1/ ds-5
$ sudo ./ds-deps-rh6_64.sh  
[ogura@centos7 OpenCL-Environment]$ sudo ./ds-deps-rh6_64.sh        // libが足りないなぁ
[sudo] password for ogura: 
Checking for glibc... installed
Checking for gtk2... installed
Checking for libstdc++... installed
Checking for alsa-lib... installed
...
Checking for atk... installed
Checking for libXt... installed
Checking for libXtst... installed
Checking for webkitgtk... Error: No matching Packages to list
not installed
Installing webkitgtk... Error: Nothing to do
Checking for glibc.i686... installed
Checking for fontconfig.i686... installed
Checking for freetype.i686... installed
...
Checking for zlib.i686... installed
Checking for libGL.so.1 (32 bit)... found

$ tar xf AOCL-18.0.0.614-linux.tar  
$ cd components  
$ ./QuartusSetup-18.0.0.614-linux.run  

パスが通らないので、設定ファイルを作る(例えばembedded.sh)

export ALTERAOCLSDKROOT=/opt/intelFPGA/18.0/hld
export QSYS_ROOTDIR=/opt/intelFPGA/18.0/quartus/sopc_builder/bin

#使用するボードの種類
export AOCL_BOARD_PACKAGE_ROOT=/opt/intelFPGA/18.0/hld/board/de1soc
export PATH=$ALTERAOCLSDKROOT/bin:$PATH
export PATH=$ALTERAOCLSDKROOT/linux64/bin:$PATH
export PATH=$ALTERAOCLSDKROOT/host/linux64/bin:$PATH
export PATH=$QSYS_ROOTDIR:$PATH
export PATH=/opt/intelFPGA/18.0/quartus/bin:$PATH

export LD_LIBRARY_PATH=$ALTERAOCLSDKROOT/host/linux64/lib
export LD_LIBRARY_PATH=$AOCL_BOARD_PACKAGE_ROOT/linux64/lib
export LD_LIBRARY_PATH=/opt/intelFPGA/18.0/embedded/ds-5/sw/gcc/arm-linux-gnueabihf/bin

#ライセンスのセットアップ
export LM_LICENSE_FILE=/opt/Downloads/license.dat
export PATH=/opt/intelFPGA/18.0/embedded/ds-5/bin:$PATH
export PATH=/opt/intelFPGA/18.0/embedded/ds-5/sw:$PATH
/opt/intelFPGA/18.0/embedded/embedded_command_shell.sh

$ . ./embedded.sh
とかでshellが起動する

#CUDA
[ここ](https://qiita.com/dyoshiha/items/5214d1076f92c9c4646f)

- GPU Board 確認  
$ lspci | pci -i nvidia
01:00.0 VGA compatible controller: NVIDIA Corporation GP107 [GeForce GTX 1050 Ti] (rev a1)
01:00.1 Audio device: NVIDIA Corporation GP107GL High Definition Audio Controller (rev a1)

- サポートされたlinuxなのか？確認
$ uname -m && cat /etc/*release
x86_64
CentOS Linux release 7.4.1708 (Core) 
NAME="CentOS Linux"
VERSION="7 (Core)"
ID="centos"
ID_LIKE="rhel fedora"
VERSION_ID="7"
PRETTY_NAME="CentOS Linux 7 (Core)"
ANSI_COLOR="0;31"
CPE_NAME="cpe:/o:centos:centos:7"
HOME_URL="https://www.centos.org/"
BUG_REPORT_URL="https://bugs.centos.org/"

CENTOS_MANTISBT_PROJECT="CentOS-7"
CENTOS_MANTISBT_PROJECT_VERSION="7"
REDHAT_SUPPORT_PRODUCT="centos"
REDHAT_SUPPORT_PRODUCT_VERSION="7"

CentOS Linux release 7.4.1708 (Core) 
CentOS Linux release 7.4.1708 (Core)

- gcc確認  CUDA8.0は4.8.2以上必要
$ gcc --version
gcc (GCC) 4.8.5 20150623 (Red Hat 4.8.5-28)

- カーネルは3.10
$ uname -r
3.10.0-693.el7.x86_64

- カーネルヘッダ確認
$ yum list installed | grep kernel
abrt-addon-kerneloops.x86_64           2.1.11-48.el7.centos            @anaconda
kernel.x86_64                          3.10.0-693.el7                  @anaconda
kernel-devel.x86_64                    3.10.0-693.el7                  @anaconda
kernel-headers.x86_64                  3.10.0-693.el7                  @anaconda
kernel-tools.x86_64                    3.10.0-693.el7                  @anaconda
kernel-tools-libs.x86_64               3.10.0-693.el7                  @anaconda

で、OKなのでCUDAをダウンロード
$ wget https://developer.nvidia.com/compute/cuda/8.0/prod/local_installers/cuda_8.0.44_linux-run

- Nouveauドライバのオフ
標準のnVIDIAドライバをオフ、、、こぇ〜っと、ディスプレイが見えなくなる？
ここまでで中断
```
