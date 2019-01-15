# CUDA with CentOS7

Anyway, 
- Don't use nouveau   
- Don't use X-Window  

### Installing NVIDIA GPU Driver

**Install CentOS7 through rufus on USB memory**  
Setup window, swap caps through Tweak tool.  
Download NVIDIA driver and CUDA previously console mode.  

[CUDA download site](https://developer.nvidia.com/cuda-downloads?target_os=Linux&target_arch=x86_64&target_distro=CentOS&target_version=7&target_type=runfilelocal)  
cuda_10.0.130_410.48_linux.run

[Driver download site](https://www.nvidia.co.jp/Download/Find.aspx?lang=jp)  
NVIDIA-Linux-x86_64-390.87.run  

[Reference Guide](http://blog.livedoor.jp/rootan2007/archives/52090548.html)  

**install kernel source**  
yum install -y kernel-headers kernel-devel  

**check graphical mode**  
systemctl get-default  

**set multi-user console mode**  
systemctl set-default multi-user.target  

**update initramfs without nouveau driver**  
mv /boot/initramfs-3.10.0-693.el7.x86_64.img /boot/initramfs-3.10.0-693.el7.x86_64-nouveau.img  
dracut --omit-drivers nouveau /boot/initramfs-3.10.0-693.el7.x86_64.img 3.10.0-693.el7.x86_64  

**disable nouveau(freeware nvidia driver)**  
echo 'blacklist nouveau' >>/etc/modprobe.d/modprobe.conf   
echo 'blacklist nouveau' >>/etc/modprobe.d/nouveau_blacklist.conf  

**apply**  
reboot  

**install nvidia_drm(product nvidia driver)**  
bash NVIDIA-Linux-x86_64-390.87.run --kernel-source-path=/usr/src/kernels/3.10.0-693.el7.x86_64  
reboot  

**check**  
lsmod | grep nouveau  
lsmod | nvidia  
```
nvidia_drm             39700  1 
nvidia_modeset       1108587  6 nvidia_drm
nvidia              14368392  248 nvidia_modeset
ipmi_msghandler        46608  2 ipmi_devintf,nvidia
drm_kms_helper        159169  2 i915,nvidia_drm
drm                   370825  5 i915,drm_kms_helper,nvidia_drm
i2c_core               40756  6 drm,i915,i2c_i801,drm_kms_helper,i2c_algo_bit,nvidia
```

### CUDA

**check graphical mode**  
systemctl get-default  

**set multi-user console mode**  
systemctl set-default multi-user.target  
reboot  
  
**install cuda toolkit**  
bash cuda_10.0.130_410.47_linux.run  

```
echo export PATH=\$PATH:/usr/local/cuda-10.0/bin/  >> .bashrc  
echo export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:/usr/local/cuda-10.0/lib64/  >>.bashrc  
```

**check with darknet**  
git clone https://github.com/k5iogura/darknet_v3  
Makefile with GPU=1 without OPENCV  
make   
```
readelf -d darknet  
 タグ        タイプ                       名前/値
 0x0000000000000001 (NEEDED)             共有ライブラリ: [libm.so.6]
 0x0000000000000001 (NEEDED)             共有ライブラリ: [libcuda.so.1] <= OK!
 0x0000000000000001 (NEEDED)             共有ライブラリ: [libcudart.so.10.0]
 0x0000000000000001 (NEEDED)             共有ライブラリ: [libcublas.so.10.0]
 0x0000000000000001 (NEEDED)             共有ライブラリ: [libcurand.so.10.0]
 0x0000000000000001 (NEEDED)             共有ライブラリ: [libstdc++.so.6]
 0x0000000000000001 (NEEDED)             共有ライブラリ: [libpthread.so.0]
 0x0000000000000001 (NEEDED)             共有ライブラリ: [libc.so.6]
...
./darknet
./darknet detect cfg/ttt5_224_160.cfg data/ttt/ttt5_224_160_final.weights data/dog.jpg
```
