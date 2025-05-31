#!/bin/sh
sudo ip tuntap add dev tap0 mode tap
sudo ip link set tap0 up
sudo ip addr add 10.0.0.1/24 dev tap0
sudo insmod ../nic_card/output/nic_card.ko
sudo ../tap/output/net_tap.elf
ping 10.0.0.2 -I tap0

