# Fibonacci in MIPS assembly
# Computes fib(10) = 55, exits with result as exit code
#
# Register usage:
#   $t0 = fib(n-2)
#   $t1 = fib(n-1)
#   $t2 = loop counter
#   $t3 = target N

    .text
    .globl __start
__start:
    li      $t3, 10         # N = 10
    li      $t0, 0          # fib(0) = 0
    li      $t1, 1          # fib(1) = 1

    beq     $t3, $zero, done_zero
    li      $t2, 1          # counter = 1

loop:
    beq     $t2, $t3, done
    add     $t4, $t0, $t1   # fib(n) = fib(n-1) + fib(n-2)
    move    $t0, $t1
    move    $t1, $t4
    addi    $t2, $t2, 1
    j       loop

done_zero:
    move    $t1, $zero

done:
    # exit syscall: $v0 = 4001 (exit), $a0 = exit code
    move    $a0, $t1        # exit code = fib(N)
    li      $v0, 4001       # Linux MIPS exit syscall
    syscall
