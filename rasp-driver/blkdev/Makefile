
TARGET=hello.ko
APP=main
obj-m := hello.o  

KDIR := /run/media/lee/backup-334GB/disk/kernel/linux-4.20.5-vscode
COMPILE := arm-linux-gnueabihf-

all:
	make -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=$(COMPILE)

clean:
	make -C $(KDIR) M=$(PWD) clean
