OUT=a.out
SRC=*.c
HRC=*.h

$(OUT): $(SRC) $(HRC)
	gcc $(SRC) -lwiringPi -lpthread -o $(OUT)

.PHONY:pi
pi:
	cd ..; make pi

.PHONY:gpu
gpu:
	cd ..; make gpu

.PHONY:io
io:
	cd ..; make io

upload: $(OUT)
	./$(OUT)

before:
	ls -d /dev/* > before

after:
	ls -d /dev/* > after && diff before after && rm before after

