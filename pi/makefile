OUT=a.out
SRC=*.c
HRC=*.h

.PHONY:pi
pi:
	cd ..; make pi

.PHONY:arduino
arduino:
	cd ..; make arduino

$(OUT): $(SRC) $(HRC)
	gcc $(SRC) -lwiringPi -o $(OUT)

upload: $(OUT)
	./$(OUT)

before:
	ls -d /dev/* > before

after:
	ls -d /dev/* > after && diff before after && rm before after

