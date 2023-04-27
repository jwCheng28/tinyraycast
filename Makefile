CC = gcc -std=c17
LIB = -lSDL2 -lm
BUILD = -w -O3
DEBUG = -g -Wall -Werror
OUT = tinyraycast

$(OUT): src/engine.c
	$(CC) $(BUILD) src/engine.c $(LIB) -o $(OUT)

debug: src/engine.c
	$(CC) $(debug) src/engine.c $(LIB) -o $(OUT)

clean:
	rm $(OUT) -f
