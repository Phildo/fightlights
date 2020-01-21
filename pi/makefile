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
	gcc $(SRC) -o $(OUT)

upload: $(OUT)
	./$(OUT)

