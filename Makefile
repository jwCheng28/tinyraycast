CC = gcc -std=c17
LIB = -lSDL2 -lm
BUILD = -w -O3
DEBUG = -g -Wall -Werror
OUT = tinyraycast

$(OUT): src/program.c
	$(CC) $(BUILD) src/program.c $(LIB) -o $(OUT)

debug: src/program.c
	$(CC) $(debug) src/program.c $(LIB) -o $(OUT)

clean:
	rm $(OUT) -f
