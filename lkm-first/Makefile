# KVERS = $(shell uname -r)
# # KVERS = 4.9.0-8-amd64

# # Kernel modules
# obj-m += lkm-first.o

# # Specify flags for the module compilation.
# EXTRA_CFLAGS= -O0

# build: kernel_modules

# kernel_modules:
# 	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules
# clean:
# 	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean




KERN_DIR = /usr/src/linux-4.19.75
PWD:=$(shell pwd)
obj-m   += lkm-first.o
COMPILE=arm-bcm2708/arm-bcm2708hardfp-linux-gnueabi/bin/arm-bcm2708hardfp-linux-gnueabi- 

all:
	make ARCH=arm CROSS_COMPILE=$(COMPILE) -C $(KERN_DIR) M=$(PWD) modules

clean:
	rm -rf *.ko *.o *.mod.o *.mod.c *.symvers  modul*  


