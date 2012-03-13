#!/bin/bash
for i in $(find .)
do
    /home/nakul/dedupcache/source/binode_hash $i
done
