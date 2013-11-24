#!/bin/bash
touch driver.txt; for i in {1..100}; do echo "go" >> driver.txt; echo "move" >> driver.txt; done; echo "quit" >> driver.txt 
