
TARGET=virt_net.ko
APP=main
obj-m := virt_net.o  

KDIR := /media/lee/_dde_data/LinuxSrc/raspbian
COMPILE := /media/lee/_dde_data/tools/arm-bcm2708/arm-bcm2708hardfp-linux-gnueabi/bin/arm-bcm2708hardfp-linux-gnueabi-

all:
	make -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=$(COMPILE)
	# $(COMPILE)gcc -g -o $(APP) main.c
	scp $(TARGET) $(APP) pi@192.168.1.120:/home/pi/mod/

clean:
	rm -rf $(APP) *.ko *.o *.mod.o *.mod.c *.symvers  modul* .tmp_versions   .*.ko.cmd  .*.mod.o.cmd  .*.o.cmd


# ifneq ($(KERNELRELEASE),)

# obj-m := hello.o

# else
# PWD :=$(shell pwd)
# KDIR := /media/lee/_dde_data/LinuxSrc/raspbian
# all:
# 	make -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
# clean:
# 	rm -rf *.ko *.o *.mod.o *.mod.c *.symvers  modul* .tmp_versions   .*.ko.cmd  .*.mod.o.cmd  .*.o.cmd

# endif