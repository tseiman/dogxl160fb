

ARCH   ?= arm
CROSS_COMPILE ?= arm-linux-gnueabi-
export ARCH CROSS_COMPILE




# flexfb-y 	:= flexfb.o

# dogxl160-y 	:= dogxl160.o



KDIR := /home/tseiman/crossdev/gnublin/lpc3131/kernel/linux-3.3.0-lpc313x

KBUILD_TARGETS	:= clean help modules modules_install


obj-m += dogxl160fb.o
dogxl160fb-objs += dogxl160fb_main.o dogxl160fb_sysfs.o

# dogxl160fb-objs += dogxl160fb_main.o dogxl160fb_sysfs.o

modules:
	make -C $(KDIR) M=$(PWD) modules

modules_install:
	make -C $(KDIR) M=$(PWD) modules_install

clean:
	make -C $(KDIR) M=$(PWD) clean
