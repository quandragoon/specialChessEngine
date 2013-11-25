#!/bin/bash
# ./generate_driver <output filename> <number of moves to make> <search depth>
touch $1; for i in $(seq 1 $2); do echo "go depth $3" >> $1; echo "move" >> $1; done; echo "quit" >> $1 
