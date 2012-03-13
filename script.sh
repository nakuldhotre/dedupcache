#!/bin/bash
for i in $(find .)
do
    /home/nd/cmps229/source/binode_hash $i
done
