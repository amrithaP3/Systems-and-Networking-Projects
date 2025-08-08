! Fall 2024 Revisions 

! This program executes pow as a test program using the LC-2200 calling convention
! Check your registers ($v0) and memory to see if it is consistent with this program

! vector table
vector0:
        .fill 0x00000000                        ! device ID 0
        .fill 0x00000000                        ! device ID 1
        .fill 0x00000000                        ! ...
        .fill 0x00000000
        .fill 0x00000000
        .fill 0x00000000
        .fill 0x00000000
        .fill 0x00000000                        ! device ID 7
        ! end vector table

main:	lea $sp, initsp                         ! initialize the stack pointer
        lw $sp, 0($sp)                          ! finish initialization


        ! TO-DO #1 ========================================================================================================
        ! Load the address of the timer_handler into the IVT using vector0
        ! =================================================================================================================
        lea $t0, vector0                        ! $t0 = address of vector0 
        lea $t1, timer_handler                  ! $t1 = address of timer_handler
        sw $t1, 0($t0)                          ! first address in vector 0 (0x0) = timer_handler


        ! TO-DO #2 ========================================================================================================
        ! Load the address of the distance_tracker_handler into the IVT (vector0 was already loaded in to-do #1)
        ! =================================================================================================================
        lea $t1, distance_tracker_handler       ! $t1 = address of distance_tracker_handler
        sw $t1, 1($t0)                          ! second address in vector 0 (0x1) = distance_tracker_handler

        lea $t0, minval
        lw $t0, 0($t0)
	lea $t1, INT_MAX 			! store 0x7FFFFFFF into minval (to initialize)
	lw $t1, 0($t1)	                  		
        sw $t1, 0($t0)

        ei                                      ! Enable interrupts

        addi $t0, $zero, 5                      ! Code to test whether min and max still work
        addi $t1, $zero, -15
        min $t2, $t0, $t1
        min $t2, $t1, $t0
        max $t2, $t0, $t1
        max $t2, $t1, $t0

        add $t0, $zero, $zero
        add $t1, $zero, $zero
        add $t2, $zero, $zero

        lea $a0, BASE                           ! load base for pow
        lw $a0, 0($a0)
        lea $a1, EXP                            ! load power for pow
        lw $a1, 0($a1)
        lea $at, POW                            ! load address of pow
        jalr $at, $ra                           ! run pow
        lea $a0, ANS                            ! load base for pow
        sw $v0, 0($a0)

        halt                                    ! stop the program here
        addi $v0, $zero, -1                     ! load a bad value on failure to halt

BASE:   .fill 2
EXP:    .fill 8
ANS:	.fill 0                                 ! should come out to 256 (BASE^EXP)

INT_MAX: .fill 0x7FFFFFFF

POW:    addi $sp, $sp, -1                       ! allocate space for old frame pointer
        sw $fp, 0($sp)

        addi $fp, $sp, 0                        ! set new frame pointer

        blt $zero, $a1, BASECHK                 ! check if $a1 is zero
        beq $zero, $zero, RET1                  ! if the exponent is 0, return 1

BASECHK:blt $zero, $a0, WORK                    ! if the base is 0, return 0
        beq $zero, $zero, RET0

WORK:   addi $a1, $a1, -1                       ! decrement the power
        lea $at, POW                            ! load the address of POW
        addi $sp, $sp, -2                       ! push 2 slots onto the stack
        sw $ra, -1($fp)                         ! save RA to stack
        sw $a0, -2($fp)                         ! save arg 0 to stack
        jalr $at, $ra                           ! recursively call POW
        add $a1, $v0, $zero                     ! store return value in arg 1
        lw $a0, -2($fp)                         ! load the base into arg 0
        lea $at, MULT                           ! load the address of MULT
        jalr $at, $ra                           ! multiply arg 0 (base) and arg 1 (running product)
        lw $ra, -1($fp)                         ! load RA from the stack
        addi $sp, $sp, 2

        beq $zero, $zero, FIN                   ! unconditional branch to FIN

RET1:   add $v0, $zero, $zero                   ! return a value of 0
	addi $v0, $v0, 1                        ! increment and return 1
        beq $zero, $zero, FIN                   ! unconditional branch to FIN

RET0:   add $v0, $zero, $zero                   ! return a value of 0

FIN:	lw $fp, 0($fp)                          ! restore old frame pointer
        addi $sp, $sp, 1                        ! pop off the stack
        jalr $ra, $zero

MULT:   add $v0, $zero, $zero                   ! return value = 0
        addi $t0, $zero, 0                      ! sentinel = 0
AGAIN:  add $v0, $v0, $a0                       ! return value += argument0
        addi $t0, $t0, 1                        ! increment sentinel
        blt $t0, $a1, AGAIN                     ! while sentinel < argument, loop again
        jalr $ra, $zero                         ! return from mult


timer_handler:

        ! TO-DO #3 ========================================================================================================
        ! Implement the timer_hander code by first doing handler setup (save $k0, enable interrupts, then save registers).
        !
        ! Next, retrieve ticks from memory, increment it by 1, then restore it back into memory
        !
        ! Finally, do the handler "teardown" (restore processor state, disable interrupts, then RETI). 
        ! =================================================================================================================
        addi $sp, $sp, -1                       ! making space for $k0 on system stack
        sw $k0, 0($sp)                          ! saving $k0

        ei                                      ! enabling interrupts

        ! saving processor state by saving registers used by processor
        addi $sp, $sp, -3                       ! making space for t registers
        sw $t0, 0($sp)                          ! saving $t0
        sw $t1, 1($sp)                          ! saving $t1
        sw $t2, 2($sp)                          ! saving $t2

        addi $sp, $sp, -2                       ! making space for s registers
        sw $s0, 0($sp)                          ! saving $s0
        sw $s1, 1($sp)                          ! saving $s1

        ! executing device code
        lea $s0, ticks                          ! $s0 = address of ticks label
        lw $s0, 0($s0)                          ! $s0 = address of ticks
        lw $s1, 0($s0)                          ! $s1 = value of ticks
        addi $s1, $s1, 1                        ! $s1 += 1
        sw $s1, 0($s0)                          ! ticks = value in $s1

        ! restoring processor state
        lw $s0, 0($sp)                          ! restoring $s0
        lw $s1, 1($sp)                          ! restoring $s1
        addi $sp, $sp, 2


        lw $t0, 0($sp)                          ! restoring $t0
        lw $t1, 1($sp)                          ! restoring $t1
        lw $t2, 2($sp)                          ! restoring $t2
        addi $sp, $sp, 3

        di                                      ! disabling interrupts

        lw $k0, 0($sp)                          ! restoring $k0
        addi $sp, $sp, 1

        reti                                    ! return from handler



distance_tracker_handler:

        ! TO-DO #4 ========================================================================================================
        ! Implement the distance_tracker_handler code by first doing handler setup (save $k0, enable interrupts, then save registers).
        !
        ! Then, retrieve the current val from the distance tracker, update maxval and minval accordingly, and then calculate the range
        !
        ! Finally, do the handler "teardown" (restore processor state, disable interrupts, then RETI).
        ! =================================================================================================================
        addi $sp, $sp, -1                       ! making space for $k0 on system stack
        sw $k0, 0($sp)                          ! saving $k0

        ei                                      ! enabling interrupts

        ! saving processor state by saving registers used by processor
        addi $sp, $sp, -3                       ! making space for t registers
        sw $t0, 0($sp)                          ! saving $t0
        sw $t1, 1($sp)                          ! saving $t1
        sw $t2, 2($sp)                          ! saving $t2

        addi $sp, $sp, -3                       ! making space for s registers
        sw $s0, 0($sp)                          ! saving $s0
        sw $s1, 1($sp)                          ! saving $s1
        sw $s2, 2($sp)                          ! saving $s2

        ! executing device code
        in $s0, 1                               ! getting current distance from distance tracker

        ! checking if we need to update max
        lea $s1, maxval                         ! $s1 = address of maxval label
        lw $s1, 0($s1)                          ! $s1 = address of maxval
        lw $s2, 0($s1)                          ! $s2 = maxval
        
        max $s1, $s0, $s2                       ! $s1 = max(curr_distance, maxval)
        beq $s1, $s0, MAX                       ! branch to updating maxval if $s1 = curr_distance

        ! checking if we need to update min
        lea $s1, minval                         ! $s1 = address of minval label
        lw $s1, 0($s1)                          ! $s1 = address of minval
        lw $s2, 0($s1)                          ! $s2 = minval
        
        min $s1, $s0, $s2                       ! $s1 = min(curr_distance, minval)
        beq $s1, $s0, MIN                       ! branch to updating minval if $s1 = curr_distance

RVAL:   lea $s0, maxval                         ! $s0 = address of maxval label
        lw $s1, 0($s0)                          ! $s0 = address of maxval
        lw $s0, 0($s1)                          ! $s0 = maxval

        lea $s1, minval                         ! $s1 = address of minval label
        lw $s1, 0($s1)                          ! $s1 = address of minval
        lw $s2, 0($s1)                          ! $s2 = minval

        nand $s2, $s2, $s2                      ! step 1 of negating minval
        addi $s2, $s2, 1                        ! step 2 of negating minval --> -minval
        add $s1, $s0, $s2                       ! $s1 = maxval - minval = range

        lea $s0, range                          ! $s1 = address of range label
        lw $s0, 0($s0)                          ! $s1 = address of range
        sw $s1, 0($s0)                          ! range = $s0
        beq $zero, $zero, TEARDOWN              ! branch to TEARDOWN                          


MAX:    lea $s2, maxval                         ! $s2 = address of maxval label
        lw $s2, 0($s2)                          ! $s2 = address of maxval
        sw $s1, 0($s2)                          ! maxval = $s1

        ! checking if we need to update minval
        lea $s1, minval                         ! $s1 = address of minval label
        lw $s1, 0($s1)                          ! $s1 = address of minval
        lw $s2, 0($s1)                          ! $s2 = minval
        
        beq $s0, $s2, RVAL                      ! branch to RVAL if min already updated (curr_distance = minval)

        min $s1, $s0, $s2                       ! $s1 = min(curr_distance, minval)
        beq $s1, $s0, MIN                       ! branch to updating minval if $s1 = curr_distance

        beq $zero, $zero, RVAL                  ! branch to RVAL

MIN:    lea $s2, minval                         ! $s2 = address of minval label
        lw $s2, 0($s2)                          ! $s2 = address of minval
        sw $s1, 0($s2)                          ! minval = $s1

        ! checking if we need to update maxval
        lea $s1, maxval                         ! $s1 = address of maxval label
        lw $s1, 0($s1)                          ! $s1 = address of maxval
        lw $s2, 0($s1)                          ! $s2 = maxval

        beq $s0, $s2, RVAL                      ! branch to RVAL if max already updated (curr_distance = maxval)
        
        max $s1, $s0, $s2                       ! $s1 = max(curr_distance, maxval)
        beq $s1, $s0, MAX                       ! branch to updating maxval if $s1 = curr_distance

        beq $zero, $zero, RVAL                  ! branch to RVAL

TEARDOWN:
        ! restoring processor state
        lw $s0, 0($sp)                          ! restoring $s0
        lw $s1, 1($sp)                          ! restoring $s1
        lw $s2, 2($sp)                          ! restoring $s2
        addi $sp, $sp, 3

        lw $t0, 0($sp)                          ! restoring $t0
        lw $t1, 1($sp)                          ! restoring $t1
        lw $t2, 2($sp)                          ! restoring $t2
        addi $sp, $sp, 3

        di                                      ! disabling interrupts

        lw $k0, 0($sp)                          ! restoring $k0
        addi $sp, $sp, 1

        reti                                    ! return from handler

initsp: .fill 0xA000
ticks:  .fill 0xFFFF
range:  .fill 0xFFFE
maxval: .fill 0xFFFD
minval: .fill 0xFFFC

