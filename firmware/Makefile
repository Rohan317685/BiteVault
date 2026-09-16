# BiteVault firmware
#
# Target: CH32V203G6U6 (QFN-28, 32 KB flash / 10 KB RAM) as fitted on the
# BiteVault PCB.  Built with ch32fun (submodule), flashed with a WCH-LinkE
# through the SWD test points (TP1 = SWDIO/PA13, TP2 = SWCLK/PA14).
#
#   make            - build firmware/bitevault.bin
#   make flash      - build and flash over WCH-Link
#   make monitor    - open the printf debug channel
#   make clean

all : build

TARGET:=bitevault
TARGET_MCU:=CH32V203
TARGET_MCU_PACKAGE:=CH32V203G6U6

CH32FUN:=ch32fun/ch32fun
MINICHLINK:=ch32fun/minichlink

# Application modules.  bitevault.c (the TARGET) is compiled automatically.
ADDITIONAL_C_FILES:= \
	ch32fun/extralibs/fsusb.c \
	src/timebase.c \
	src/led.c \
	src/button.c \
	src/i2c.c \
	src/atecc608.c \
	src/ctaphid.c \
	src/u2f.c

EXTRA_CFLAGS:=-Isrc

ifeq ("$(wildcard $(CH32FUN)/ch32fun.mk)","")
$(error ch32fun is missing - run: git submodule update --init --recursive)
endif

include $(CH32FUN)/ch32fun.mk

flash : cv_flash
clean : cv_clean
