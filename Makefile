CC = gcc
TARGET = mips_executable/output

all: $(TARGET)

# Build the compiler
build/compiler: src/main.c
	$(CC) -o build/compiler src/*.c

# Run compiler to emit MIPS assembly
mips_asm/output.s: build/compiler
	build/compiler > mips_asm/output.s

# Assemble and link into static MIPS executable
$(TARGET): mips_asm/output.s
	clang --target=mips-unknown-linux-gnu -nostdlib -static -fuse-ld=lld -Wl,-e,__start mips_asm/output.s -o $(TARGET)

clean:
	rm -f build/compiler mips_asm/output.s $(TARGET)

.PHONY: all clean
