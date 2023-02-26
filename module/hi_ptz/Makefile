CURRENT_PATH := $(shell pwd)
KERNELDIR := $(CURRENT_PATH)/../../../../out/gk7205v200/linux-4.9.y

ARCH=arm
CROSS_COMPILE=arm-gcc6.3-linux-uclibceabi-
export  ARCH  CROSS_COMPILE

obj-m := hi_ptz.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean

