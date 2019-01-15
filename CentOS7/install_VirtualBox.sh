# prepare package needed
yum install -y kernel-devel kernel-headers dkms
yum update

# install Key
wget http://download.virtualbox.org/virtualbox/debian/oracle_vbox.asc
rpm --import oracle_vbox.asc

# install Repository
wget http://download.virtualbox.org/virtualbox/rpm/el/virtualbox.repo -O /etc/yum.repos.d/virtualbox.repo
yum update

# install VirtualBox
yum install -y VirtualBox-5.2

