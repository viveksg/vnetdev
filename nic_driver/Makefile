BUILD_DIR=$(PWD)/output
BUILD_DIR_BB=$(PWD)/output_beaglebone/
obj-m := nic_driver.o
EXTRA_CFLAGS += -I$(PWD)/../nic_card/includes
ARCH=arm
CROSS_COMPILE=arm-none-linux-gnueabihf-
KERN_DIR = /home/vivsg/projects/bblinux/v6.12.20
HOST_KERN_DIR = /lib/modules/$(shell uname -r)/build/

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) modules
	mkdir -p $(BUILD_DIR_BB)
	mv $(PWD)/.*.cmd $(BUILD_DIR_BB)
	mv $(PWD)/*ko $(BUILD_DIR_BB)
	mv $(PWD)/*symvers $(BUILD_DIR_BB)
	mv $(PWD)/*order $(BUILD_DIR_BB)
	mv $(PWD)/*o $(BUILD_DIR_BB)
	mv $(PWD)/*mod* $(BUILD_DIR_BB)
clean:
	make -C $(HOST_KERN_DIR) M=$(PWD) clean
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) clean
	rm -rf $(BUILD_DIR)
	rm -rf $(BUILD_DIR_BB)
help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) help
host:
	make -C $(HOST_KERN_DIR)  M=$(PWD) modules
	mkdir -p $(BUILD_DIR)
	mv $(PWD)/.*.cmd $(BUILD_DIR)
	mv $(PWD)/*ko $(BUILD_DIR)
	mv $(PWD)/*symvers $(BUILD_DIR)
	mv $(PWD)/*order $(BUILD_DIR)
	mv $(PWD)/*o $(BUILD_DIR)
	mv $(PWD)/*mod* $(BUILD_DIR)