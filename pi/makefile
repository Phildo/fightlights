OUT=fightlights
SRC=*.c
HRC=*.h

$(OUT): $(SRC) $(HRC)
	gcc -g3 $(SRC) -lm -lwiringPi -lpthread -lasound -lsndfile -o $(OUT)

debug: $(OUT)
	gdb $(OUT)

run: $(OUT)
	./$(OUT)

.PHONY:pi
pi:
	cd ..; make pi

.PHONY:arduino
arduino:
	cd ..; make arduino

.PHONY:gpu
gpu:
	cd ..; make gpu

.PHONY:mio
mio:
	cd ..; make mio

.PHONY:btn
btn:
	cd ..; make btn

upload: $(OUT)
	./$(OUT)

before:
	ls -d /dev/* > before

after:
	ls -d /dev/* > after && diff before after && rm before after

