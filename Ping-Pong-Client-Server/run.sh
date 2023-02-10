#! /bin/bash

for i in {0..655}
do 
    (./client onyx.clear.rice.edu 18137 $((100*i+18)) 10000) &>> times2.txt
done 