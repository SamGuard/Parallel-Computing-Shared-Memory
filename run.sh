#!/bin/bash

FILENAME="main"

rm "./$FILENAME"

gcc main.c -o $FILENAME -lpthread
  
if test -f "$FILENAME"; then
    ./main 8 64 64 0.000001  > output.txt
else
  echo "Could not compile"
fi
