obj-m += zenbook_ec.o

KDIR ?= /lib/modules/$(shell uname -r)/build
BUILD_DIR ?= $(PWD)/build

all:
		make -C $(KDIR) M=$(PWD) modules
		mkdir -p $(BUILD_DIR)
		mv -f *.o *.ko *.mod *.mod.c *.cmd Module.symvers modules.order .*.cmd $(BUILD_DIR)/ 2>/dev/null || true

clean: 
		make -C $(KDIR) M=$(PWD) clean
		rm -rf $(BUILD_DIR)