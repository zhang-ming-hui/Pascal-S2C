#!/bin/bash

if [ $# -ne 1 ]; then
	ls ../tests/errorcases
	exit
fi

for file in ../tests/errorcases/$1/*; do
	if [ ${file##*.} == pas ]; then
		echo "-------------------- cat $file --------------------"
		cat $file
		echo "------------------ compile $file ------------------"
		../build/pascal_s2c $file
	fi
done	
