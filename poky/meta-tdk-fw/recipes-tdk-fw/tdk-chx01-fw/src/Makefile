CC = ~/raspberrypi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-gcc

all: tdk-chx01-fw

app : tdk-chx01-fw.c
	$(CC) -o $@ $^
clean :
	rm tdk-chx01-fw

