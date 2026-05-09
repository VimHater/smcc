CC = clang

# Build the compiler
build/compiler: src/main.c
	$(CC) -o build/compiler src/*.c

.PHONY: all clean
