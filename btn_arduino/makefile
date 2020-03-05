ARDUINO_DIR = /usr/share/arduino

USER_LIB_PATH = $(HOME)/sketchbook/libraries

#use for old nanos (box buttons)
#BOARD_TAG = nano328
#BOARD_SUB = atmega328old

#use for new nanos (big buttons)
BOARD_TAG = uno
BOARD_SUB = atmega328

include /usr/share/arduino/Arduino.mk

.PHONY:arduino
arduino:
	cd ..; make arduino

.PHONY:btn
btn:
	cd ..; make btn

.PHONY:mio
mio:
	cd ..; make mio

.PHONY:gpu
gpu:
	cd ..; make gpu

.PHONY:pi
pi:
	cd ..; make pi

