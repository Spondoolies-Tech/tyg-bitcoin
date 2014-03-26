#
# Since version 1.6 the asynchronous mode has been
# disabled by default. To re-enable it uncomment the
# corresponding CFLAG.
#
CRYPTODEV_CFLAGS ?= #-DENABLE_ASYNC
VERSION = 1.6
PREFIX =


ccflags-y			+= -I$(src) -I$(src)/crypto $(CRYPTODEV_CFLAGS)

cryptodev-objs = ioctl.o main.o cryptlib.o authenc.o zc.o util.o

obj-m += cryptodev.o

