ARDUINO_DIR = /usr/share/arduino

USER_LIB_PATH = $(HOME)/sketchbook/libraries

ARDUINO_PORT = /dev/ttyUSB0
BOARD_TAG = uno #nano328
BOARD_SUB = atmega328old

include /usr/share/arduino/Arduino.mk

.PHONY:gpu
gpu:
	cd ..; make gpu

.PHONY:io
io:
	cd ..; make io

.PHONY:pi
pi:
	cd ..; make pi

