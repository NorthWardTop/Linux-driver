
TARGET=hello.ko
APP=main
obj-m := hello.o  

KDIR := /media/lee/_dde_data/LinuxSrc/raspbian
COMPILE := /media/lee/_dde_data/tools/arm-bcm2708/arm-bcm2708hardfp-linux-gnueabi/bin/arm-bcm2708hardfp-linux-gnueabi-

all:
	make -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=$(COMPILE)
	$(COMPILE)$(CC) -g -o $(APP) main.c
	scp $(TARGET) $(APP) pi@raspberrypi.lan:/home/pi/mod

clean:
	rm -rf $(APP) *.ko *.o *.mod.o *.mod.c *.symvers  modul* .tmp_versions   .*.ko.cmd  .*.mod.o.cmd  .*.o.cmd

