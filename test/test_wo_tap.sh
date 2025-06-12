#!/bin/sh
#prepare nic_card
cd ..
cd nic_card
make clean
make host
sudo insmod output/nic_card.ko
#prepare nic_driver
cd ..
cd nic_driver
make clean
make host
sudo insmod output/nic_driver.ko
#test_transfer
cd ..
cd tap
#sudo ip addr del 10.0.0.2/24 dev tap0
#sudo ip link set tap0 down
#sudo ip tuntap del dev tap0 mode tap

#sudo ip addr del 10.0.0.2/24 dev vnic0
#sudo ip link set vnic0 down
sudo ip link set tap0 down
sudo ip tuntap del dev tap0 mode tap
sudo ip tuntap add dev tap0 mode tap
sudo ip link set tap0 up

sudo ip addr add 10.0.0.2/24 dev vnic0
sudo ip link set vnic0 up

sudo output/net_tap.elf
cd ..
#ping 10.0.0.2 

