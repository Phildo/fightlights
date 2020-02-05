.PHONY:arduino
arduino:
	./upload_all_arduinos.sh

.PHONY:gpu
gpu:
	cd gpu_arduino; ARDUINO_PORT=/dev/ttyUSB* make upload

.PHONY:mio
mio:
	cd mio_arduino; ARDUINO_PORT=/dev/ttyUSB* make upload

.PHONY:btn
btn:
	cd btn_arduino; ARDUINO_PORT=/dev/ttyUSB* make upload

.PHONY:pi
pi:
	cd pi; make upload

