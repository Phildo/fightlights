.PHONY:gpu
gpu:
	cd gpu_arduino; make upload

.PHONY:io
io:
	cd io_arduino; make upload

.PHONY:pi
pi:
	cd pi; make upload

