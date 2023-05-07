CC = gcc -std=c17
LIB = -lSDL2 -lm
BUILD = -w -O3
DEBUG = -g -Wall -Werror
NAME = tinyraycast
BIN = bin
WEB = web
OUT = $(BIN)/$(NAME)
JS = $(NAME).js

$(OUT): src/engine.c
	$(CC) $(BUILD) src/engine.c $(LIB) -o $(OUT)

debug: src/engine.c
	$(CC) $(debug) src/engine.c $(LIB) -o $(OUT)

wasm: src/engine.c
	emcc src/engine.c -o $(WEB)/$(JS) -Oz -lm --bind -s USE_SDL=2 -s WASM=1

run:
	./$(OUT)

webtest:
	emrun $(WEB)/index.html

clean:
	rm $(OUT) -f
