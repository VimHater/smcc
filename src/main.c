#include <stdio.h>

int main(int argc, char** argv)
{
    int ret_code = 1;
    printf(
        ".text\n"
        ".globl __start\n"
        "__start:\n"
        "li $v0, 4001\n"
        "li $a0, %d\n"
        "syscall\n"
        ,
        ret_code
    );

}
