all:
	g++ -o usb_test          usb_test.c          -L ../include
#	g++ -o usb_test_read     usb_test_read.c     -L ../include
#	g++ -o usb_test_read_ok     usb_test_read_ok.c     -L ../include
#	g++ -o 08_cybulk          08_cybulk.c          -L ../lib -l cyusb -l pthread
clean:
	rm -f usb_test
#	rm -f usb_test_read
#	rm -f 08_cybulk

help:
	@echo	'make		would compile all source programs in this directory
	@echo	'make clean	would remove all object code in this directory
