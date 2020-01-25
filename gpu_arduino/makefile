ARDUINO_DIR = /usr/share/arduino

USER_LIB_PATH = $(HOME)/sketchbook/libraries

ARDUINO_PORT = /dev/ttyUSB0
BOARD_TAG = uno
BOARD_SUB = atmega328

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

