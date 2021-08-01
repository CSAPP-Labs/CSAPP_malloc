#! /usr/bin/bash
# Run this test script in CSAPP/cachelab


## check transpose functions
make clean;
make;
# ./test-trans -M 32 -N 32
#./test-trans -M 64 -N 64
# ./test-trans -M 61 -N 67

#./csim-ref -v -s 5 -E 1 -b 5 -t ./trace.f0

## check both test-csim and test-trans
 ./driver.py

