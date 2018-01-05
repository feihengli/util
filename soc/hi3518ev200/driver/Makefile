

LINUX_ROOT?=/home/lifeiheng/work/hisi_sdk/Hi3518E_SDK_V1.0.3.0/osdrv/opensource/kernel/linux-3.4.y

PWD := $(shell pwd)

obj-m := board_ctl.o
gpioi2c-y := board_ctl.o

all:
	make ARCH=arm CROSS_COMPILE=arm-hisiv300-linux-  -C $(LINUX_ROOT) M=$(PWD) modules
	cp board_ctl.ko /mnt/hgfs/nfs/bpc/ -af
clean:
	make -C $(LINUX_ROOT) M=$(PWD) clean
