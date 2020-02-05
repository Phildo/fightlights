#!/bin/bash

if [ "$(echo /dev/ttyUSB*)" == '/dev/ttyUSB*' ]; then echo "No serial ports found."; exit 1; fi

for i in /dev/ttyUSB*; do
  echo -n "checking $i... "
  if [ -c $i ]; then
    AID="`pingserial $i 1000000 CMD_0`";
    echo $AID
      if [ "$AID" == "GPU" ]; then cd gpu_arduino; ARDUINO_PORT=$i make upload; cd -;
    elif [ "$AID" == "MIO" ]; then cd mio_arduino; ARDUINO_PORT=$i make upload; cd -;
    elif [ "$AID" == "BTN" ]; then cd btn_arduino; ARDUINO_PORT=$i make upload; cd -;
    else echo "not a project arduino."
    fi
  else
    echo "not a device."
  fi
done

