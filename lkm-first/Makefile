KVERS = $(shell uname -r)
# KVERS = 4.9.0-8-amd64

# Kernel modules
obj-m += lkm-first.o

# Specify flags for the module compilation.
EXTRA_CFLAGS= -O0

build: kernel_modules

kernel_modules:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules
clean:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean
