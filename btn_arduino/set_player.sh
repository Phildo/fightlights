#!/bin/bash

sed -i "s@#define PLAYER .*@#define PLAYER $1 //0 or 1@" -i btn_arduino.ino 

