#! /usr/bin/bash
# Run this test script in CSAPP/archlab/sim/pipe.
# The common directory for all adjacent tasks is sim.

### Specify which versions are being built.
VER=btfnt				# psim version
SIMTYPE=../pipe/psim		# sim type
SIMTEST=testpsim		# sim test type
CODEPATH=../y86-code/btfnttst.ys	# test code
OBJPATH=../y86-code/btfnttst.yo	# test obj code
ASSM=../misc/yas		# assembler
ISIM=../misc/yis		# instruction simulator


### Builds
## Build piped processor simulator psim via pipe directory Makefile
make clean;
make psim VERSION=$VER;

## Compile test code. Compiles silently.
$ASSM $CODEPATH


### Tests
## Compare test code run on psim and YIS, mid verbosity
$SIMTYPE -t $OBJPATH -v 1;

## Run benchmark programs on psim
 cd ../y86-code;
 make $SIMTEST;
 cd ../pipe;

## Run regression tests on psim
 cd ../ptest;
 make SIM=$SIMTYPE;
 cd ../pipe;

## Run test code on psim, GUI mode. Invokes silently.
# $SIMTYPE -g $OBJPATH &


