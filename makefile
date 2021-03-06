.PHONY:arduino
arduino:
	@./upload_all_arduinos.sh

.PHONY:force
force: #manually edit this to force an arduino on a port
	#cd gpu_arduino; ARDUINO_PORT=/dev/ttyACM0 make upload
	#cd mio_arduino; ARDUINO_PORT=/dev/ttyUSB* make upload
	cd btn_arduino; ARDUINO_PORT=/dev/ttyUSB* make upload

.PHONY:list
	@cd gpu_arduino; for i in /dev/ttyUSB* /dev/ttyACM*; do if [ "GPU" = "`pingserial $$i 1000000 CMD_0`" ]; then ARDUINO_PORT=$$i make upload; break; fi; done

.PHONY:gpu
gpu:
	@cd gpu_arduino; for i in /dev/ttyUSB* /dev/ttyACM*; do if [ "GPU" = "`pingserial $$i 1000000 CMD_0`" ]; then ARDUINO_PORT=$$i make upload; break; fi; done

.PHONY:mio
mio:
	@cd mio_arduino; for i in /dev/ttyUSB* /dev/ttyACM*; do if [ "MIO" = "`pingserial $$i 1000000 CMD_0`" ]; then ARDUINO_PORT=$$i make upload; break; fi; done

.PHONY:btn
btn:
	@cd btn_arduino; for i in /dev/ttyUSB* /dev/ttyACM*; do RSP="`pingserial $$i 1000000 CMD_0`"; if [ "BTN0" = "$$RSP" ]; then ./set_player.sh 0; ARDUINO_PORT=$$i make upload; elif [ "BTN1" = "$$RSP" ]; then ./set_player.sh 1; ARDUINO_PORT=$$i make upload; fi; done

.PHONY:pi
pi:
	@cd pi; make upload

