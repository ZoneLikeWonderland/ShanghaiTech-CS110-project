.global cudnn


cudnn:
addi sp, sp, -8
sw ra, 0(sp)
sw s1, 4(sp)

mv s1, a0
lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw a0, 0(s1)
call LCD_WR_DATA
addi s1, s1, 2

lw s1, 4(sp)
lw ra, 0(sp)
addi sp, sp, 8
ret



.global quicksort

quicksort:
addi sp, sp, -8
sw ra, 0(sp)
sw s1, 4(sp)

mv s1, a0
mv a0, s1
call cudnn
addi s1, s1, 60

mv a0, s1
call cudnn
addi s1, s1, 60

mv a0, s1
call cudnn
addi s1, s1, 60

mv a0, s1
call cudnn
addi s1, s1, 60

mv a0, s1
call cudnn
addi s1, s1, 60

mv a0, s1
call cudnn
addi s1, s1, 60

mv a0, s1
call cudnn
addi s1, s1, 60

mv a0, s1
call cudnn
addi s1, s1, 60

mv a0, s1
call cudnn
addi s1, s1, 60

mv a0, s1
call cudnn
addi s1, s1, 60

mv a0, s1
call cudnn
addi s1, s1, 60

mv a0, s1
call cudnn
addi s1, s1, 60

mv a0, s1
call cudnn
addi s1, s1, 60

mv a0, s1
call cudnn
addi s1, s1, 60

mv a0, s1
call cudnn
addi s1, s1, 60




lw s1, 4(sp)
lw ra, 0(sp)
addi sp, sp, 8
ret

.global montecarlo

montecarlo:
addi sp, sp, -8
sw ra, 0(sp)
sw s1, 4(sp)

li a0,0
li a1,0
li a2,29
li a3,14
call LCD_Address_Set

la a0,panel
call quicksort

li a0,130
li a1,0
li a2,159
li a3,14
call LCD_Address_Set

la a0,panel
call quicksort

lw s1, 4(sp)
lw ra, 0(sp)
addi sp, sp, 8
ret


.global Qlearning

Qlearning:
li a5,8192
addi a5,a5,-597
beqz a0,noright
li a0,6
bgt a1,a5,splay_tree
li a5,4096
addi a5,a5,-1682
li a0,5
bgt a1,a5,splay_tree
li a5,1303
li a0,4
bgt a1,a5,splay_tree
li a5,767
li a0,3
bgt a1,a5,splay_tree
li a5,414
li a0,2
bgt a1,a5,splay_tree
li a5,131
li a0,1
bgt a1,a5,splay_tree
li a5,-130
li a0,0
bge a1,a5,splay_tree
li a5,-413
li a0,23
bge a1,a5,splay_tree
li a5,-766
li a0,22
bge a1,a5,splay_tree
li a5,-1302
li a0,21
bge a1,a5,splay_tree
li a5,-4096
addi a5,a5,1683
li a0,20
bge a1,a5,splay_tree
li a5,-8192
addi a5,a5,598
slt a0,a1,a5
xori a0,a0,1
addi a0,a0,18
ret
noright:
li a0,18
bgt a1,a5,splay_tree
li a5,4096
addi a5,a5,-1682
li a0,17
bgt a1,a5,splay_tree
li a5,1303
li a0,16
bgt a1,a5,splay_tree
li a5,767
li a0,15
bgt a1,a5,splay_tree
li a5,414
li a0,14
bgt a1,a5,splay_tree
li a5,131
li a0,13
bgt a1,a5,splay_tree
li a5,-130
li a0,12
bge a1,a5,splay_tree
li a5,-413
li a0,11
bge a1,a5,splay_tree
li a5,-766
li a0,10
bge a1,a5,splay_tree
li a5,-1302
li a0,9
bge a1,a5,splay_tree
li a5,-4096
addi a5,a5,1683
li a0,8
bge a1,a5,splay_tree
li a0,-8192
addi a0,a0,598
slt a0,a1,a0
xori a0,a0,1
addi a0,a0,6
splay_tree:
ret

.global suffix_automaton
suffix_automaton:
li a5,1376
beqz a0,exgcd
li a0,4
bgt a1,a5,Mobius_inversion
li a5,325
li a0,2
bgt a1,a5,Mobius_inversion
li a5,-324
li a0,0
bge a1,a5,Mobius_inversion
li a5,-1375
li a0,8
blt a1,a5,fft
Mobius_inversion:
ret
exgcd:
li a0,7
bgt a1,a5,Mobius_inversion
li a5,325
li a0,9
bgt a1,a5,Mobius_inversion
li a5,-324
li a0,1
bge a1,a5,Mobius_inversion
li a5,-1375
li a0,3
bge a1,a5,Mobius_inversion
li a0,5
ret
fft:
li a0,6
ret