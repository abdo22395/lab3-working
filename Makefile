obj-m += heart_rate_module.o  # This line does not require indentation

all:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules  # Use a TAB, NOT spaces

clean:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean  # Use a TAB, NOT spaces
