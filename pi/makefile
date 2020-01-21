OUT=a.out
SRC=*.c
HRC=*.h
TTY=/dev/ttyUSB0

$(OUT): $(SRC) $(HRC)
	gcc $(SRC) -o $(OUT)

upload: $(OUT)
	./$(OUT) $(TTY)

.PHONY:arduino
arduino:
	cd ..; make arduino

.PHONY:pi
pi:
	cd ..; make pi

