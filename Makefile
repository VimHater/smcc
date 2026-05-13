CC = clang
build/compiler: src/main.c
	$(CC) -o build/compiler src/*.c -Wall -Wextra
