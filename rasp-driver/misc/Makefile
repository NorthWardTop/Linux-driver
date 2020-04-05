# KVERS = $(shell uname -r)

# # Kernel modules
# obj-m += plat-misc.o plat-miscdev.o

# # Specify flags for the module compilation.
# EXTRA_CFLAGS=-g -O0

# build: kernel_modules

# kernel_modules:
# 	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules

# clean:
# 	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean




TARGET=plat-misc.ko plat-miscdev.ko
APP=main
obj-m := plat-misc.o plat-miscdev.o

KDIR := /media/lee/_dde_data/LinuxSrc/raspbian
COMPILE := /media/lee/_dde_data/tools/arm-bcm2708/arm-bcm2708hardfp-linux-gnueabi/bin/arm-bcm2708hardfp-linux-gnueabi-

all:
	make -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=$(COMPILE)
	# $(COMPILE)$(CC) -g -o $(APP) main.c
	scp $(TARGET) pi@raspberrypi.lan:/home/pi/plat-misc/

clean:
	rm -rf $(APP) *.ko *.o *.mod.o *.mod.c *.symvers  modul* .tmp_versions   .*.ko.cmd  .*.mod.o.cmd  .*.o.cmd .cache.*
