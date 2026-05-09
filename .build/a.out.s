.set noreorder
.option pic0
.text
.globl __start
__start:
    jal main
    move $a0, $v0
    li $v0, 4001
    syscall

main:
    addiu $sp, $sp, -4
    sw $ra, 0($sp)
    li $t0, 72
    move $a0, $t0
    li $v0, 4004
    addiu $sp, $sp, -4
    sb $t0, 0($sp)
    move $a1, $sp
    li $a0, 1
    li $a2, 1
    syscall
    addiu $sp, $sp, 4
    li $t0, 101
    move $a0, $t0
    li $v0, 4004
    addiu $sp, $sp, -4
    sb $t0, 0($sp)
    move $a1, $sp
    li $a0, 1
    li $a2, 1
    syscall
    addiu $sp, $sp, 4
    li $t0, 108
    move $a0, $t0
    li $v0, 4004
    addiu $sp, $sp, -4
    sb $t0, 0($sp)
    move $a1, $sp
    li $a0, 1
    li $a2, 1
    syscall
    addiu $sp, $sp, 4
    li $t0, 108
    move $a0, $t0
    li $v0, 4004
    addiu $sp, $sp, -4
    sb $t0, 0($sp)
    move $a1, $sp
    li $a0, 1
    li $a2, 1
    syscall
    addiu $sp, $sp, 4
    li $t0, 111
    move $a0, $t0
    li $v0, 4004
    addiu $sp, $sp, -4
    sb $t0, 0($sp)
    move $a1, $sp
    li $a0, 1
    li $a2, 1
    syscall
    addiu $sp, $sp, 4
    li $t0, 32
    move $a0, $t0
    li $v0, 4004
    addiu $sp, $sp, -4
    sb $t0, 0($sp)
    move $a1, $sp
    li $a0, 1
    li $a2, 1
    syscall
    addiu $sp, $sp, 4
    li $t0, 87
    move $a0, $t0
    li $v0, 4004
    addiu $sp, $sp, -4
    sb $t0, 0($sp)
    move $a1, $sp
    li $a0, 1
    li $a2, 1
    syscall
    addiu $sp, $sp, 4
    li $t0, 111
    move $a0, $t0
    li $v0, 4004
    addiu $sp, $sp, -4
    sb $t0, 0($sp)
    move $a1, $sp
    li $a0, 1
    li $a2, 1
    syscall
    addiu $sp, $sp, 4
    li $t0, 114
    move $a0, $t0
    li $v0, 4004
    addiu $sp, $sp, -4
    sb $t0, 0($sp)
    move $a1, $sp
    li $a0, 1
    li $a2, 1
    syscall
    addiu $sp, $sp, 4
    li $t0, 108
    move $a0, $t0
    li $v0, 4004
    addiu $sp, $sp, -4
    sb $t0, 0($sp)
    move $a1, $sp
    li $a0, 1
    li $a2, 1
    syscall
    addiu $sp, $sp, 4
    li $t0, 100
    move $a0, $t0
    li $v0, 4004
    addiu $sp, $sp, -4
    sb $t0, 0($sp)
    move $a1, $sp
    li $a0, 1
    li $a2, 1
    syscall
    addiu $sp, $sp, 4
    li $t0, 10
    move $a0, $t0
    li $v0, 4004
    addiu $sp, $sp, -4
    sb $t0, 0($sp)
    move $a1, $sp
    li $a0, 1
    li $a2, 1
    syscall
    addiu $sp, $sp, 4
    li $t0, 0
    move $v0, $t0
    lw $ra, 0($sp)
    addiu $sp, $sp, 4
    jr $ra
