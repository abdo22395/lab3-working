# Ange namnet på kernelmodulen här
obj-m += sram_uart_module.o

# Ange väg till Raspberry Pi:s kernel sources (automatiskt upptäcks med uname)
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Bygg mål
all:
	@echo "Building the kernel module..."
	make -C $(KDIR) M=$(PWD) modules

# Rensa byggfiler
clean:
	@echo "Cleaning up..."
	make -C $(KDIR) M=$(PWD) clean
