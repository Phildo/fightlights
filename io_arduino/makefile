ARDUINO_DIR = /usr/share/arduino

USER_LIB_PATH = $(HOME)/sketchbook/libraries

ARDUINO_PORT = /dev/ttyUSB1
BOARD_TAG = nano328
BOARD_SUB = atmega328old

include /usr/share/arduino/Arduino.mk

.PHONY:io
io:
	cd ..; make io

.PHONY:gpu
gpu:
	cd ..; make gpu

.PHONY:pi
pi:
	cd ..; make pi

