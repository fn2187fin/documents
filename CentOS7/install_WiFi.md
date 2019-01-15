# Cenos7 WiFi 
### BUFFALO WLI-UC-GNM2S work fine###
***For Cenos7 WiFi*** 

- Just only insert to USB port, it works fine  
Reference : [famous story](http://ich.hatenadiary.com/entry/2018/03/27/085236)

```
$ lsusb
Bus 003 Device 011: ID 0411:01ee BUFFALO INC. (formerly MelCo., Inc.)
WLI-UC-GNM2 Wireless LAN Adapter [Ralink RT3070]

$ ifconfig
wlp0s20u5: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 192.168.11.23  netmask 255.255.255.0  broadcast 192.168.11.255
        inet6 fe80::47c3:6067:1f42:b695  prefixlen 64  scopeid 0x20<link>
        ether 84:af:ec:53:97:28  txqueuelen 1000  (Ethernet)
        RX packets 10599  bytes 15368573 (14.6 MiB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 5788  bytes 515830 (503.7 KiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

$ lsmod
rt2800usb              31203  0 
rt2x00usb              20778  1 rt2800usb
rt2800lib             106595  1 rt2800usb
crc_ccitt              12707  1 rt2800lib
rt2x00lib              70005  3 rt2x00usb,rt2800lib,rt2800usb
mac80211              714741  3 rt2x00lib,rt2x00usb,rt2800lib
```

Linux driver for rt2800usb(WLI-UC-GNM2-Chip) is invoked.  

### Elecom WDC-150SU2MBK not work###
***For Cenos7 WiFi*** 
***For Ubuntu 16.04***  
Just insert WDC-150SU2M into USB port. 
```
$ lsmod | grep 8188
r8188eu               425984  0
cfg80211              622592  1 r8188eu
```
WiFi seems ready to go.  
Gnome Window Manager show Usable WiFi Station near area like a Windows environment. Connect Your WiFi AccessPoint.  
```
// Check Scan
$ iwconfig
...

$ iwlist wlxbc5c4cd7a5ec scan | head -30
wlxbc5c4cd7a5ec  Scan completed :
          Cell 01 - Address: xx:xx:xx:xx:xx
                    ESSID:"yyyyyyyyyyyy"
                    Protocol:IEEE 802.11bgn
                    Mode:Master
                    Frequency:2.442 GHz (Channel 7)
                    Encryption key:on
                    Bit Rates:144 Mb/s
                    Quality=100/100  Signal level=90/100  
          Cell 02 - 
          ...
```
Nothing to do, WiFi environment is all ready fine on Ubuntu 16.04 just insert USB Donggle.  

***For CentOS7***  
- Reference  
[UbuntuでElecom WDC-150SU2Mを使う](https://twinbird-htn.hatenablog.com/entry/2015/12/27/231643)  
[Raspberry Pi B+ モデルで無線LANアダプタ(Elecom WDC-150SU2M)を利用する](https://qiita.com/kompiro/items/da6ae0ca16a7705f21a0)  

- Insert WDC-150SU2MBK  into USB port and,  
```
$ dmesg | tail
```
Compile Driver and insmod by git,
```
$ git clone https://github.com/lwfinger/rtl8188eu
$ cd rtl8188eu
$ make all
# make install
# modprobe
# insmod 8188eu.ko
```
or download binary without compile by wget,
```
mkdir 8818 ; cd 8818
wget http://www.fars-robotics.net/8188eu-4.4.8-v7-881.tar.gz
tar xzf 8188eu-4.4.8-v7-881.tar.gz
./install.sh
```
- Install iwconfig to use iwconfig,  
```
# yum install -y wireless-tools
# iwconfig
wlp0s20u5  unassociated  ESSID:""  Nickname:"<WIFI@REALTEK>"
          Mode:Auto  Frequency=2.412 GHz  Access Point: Not-Associated   
          Sensitivity:0/0  
          Retry:off   RTS thr:off   Fragment thr:off
          Power Management:off
          Link Quality=0/100  Signal level=0 dBm  Noise level=0 dBm
          Rx invalid nwid:0  Rx invalid crypt:0  Rx invalid frag:0
          Tx excessive retries:0  Invalid misc:0   Missed beacon:0
lo        no wireless extensions.
virbr0-nic  no wireless extensions.
virbr0    no wireless extensions.
enp3s0    no wireless extensions.
```
