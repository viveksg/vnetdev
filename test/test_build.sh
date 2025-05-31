#!/bin/sh
cd ..
cd nic_card
make clean
make host
sudo insmod output/nic_card.ko
cd ..
cd nic_driver
make clean
make host
sudo insmod output/nic_driver.ko