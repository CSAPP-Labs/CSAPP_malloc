#! /usr/bin/bash
# Run this test script in CSAPP/cachelab

### debugging
ulimit -c unlimited
sudo sysctl -w kernel.core_pattern=/home/an/Desktop/CSAPP/cachelab/core-%e;

### preparation outline
## compile simulator
(gcc -Og -o csim csim.c cachelab.c) #&& (./csim -s 4 -E 2 -b 4 -t traces/yi.trace)

## run sim: need to compare this with ./csim-ref results
#./csim -s 1 -E 1 -b 1 -t traces/yi2.trace
#./csim -s 4 -E 2 -b 4 -t traces/yi.trace
#./csim -s 2 -E 1 -b 4 -t traces/dave.trace
#./csim -s 2 -E 1 -b 3 -t traces/trans.trace
#./csim -s 2 -E 2 -b 3 -t traces/trans.trace
./csim -s 2 -E 4 -b 3 -t traces/trans.trace 
./csim -s 5 -E 1 -b 5 -t traces/trans.trace
./csim -s 5 -E 1 -b 5 -t traces/long.trace

### assignment checks

## build lab with csim.c and trans.c, check sim correctness
## but no need to generate tar file each time?
# make clean; make;
# ./test-csim

## check transpose functions
# ./test-trans -M 32 -N 32
# ./test-trans -M 64 -N 64
# ./test-trans -M 61 -N 67

## check both test-csim and test-trans
# ./driver.py

