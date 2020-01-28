.PHONY:gpu
gpu:
	cd gpu_arduino; make upload

.PHONY:io
io:
	cd io_arduino; make upload

.PHONY:btn
btn:
	cd btn_arduino; make upload

.PHONY:pi
pi:
	cd pi; make upload

