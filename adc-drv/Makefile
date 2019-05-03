

# m是module的意思，+= 追加赋值。?= 或 := 区别。led_drv.o---驱动远程序的目标文件。
obj-m += adc-drv.o
# 内核源码路径
KERNELDIR:=/home/lyh/my-work/6818/6818GEC/kernel

# 交叉工具链
CROSS_COMPILE:=arm-linux-
# 当前路径
PWD:=$(shell pwd)

# 去内核源码目录下，找到内核源码的Makefile文件，并将ARCH和CROSS_COMPILE变量传递给Makefile文件。
# 利用内核源码的Makefile文件来编译当前路径下的驱动源文件，将驱动源文件编译成一个ko。
default:
	$(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf *.o *.order .*.cmd *.ko *.mod.c *.symvers .tmp_versions