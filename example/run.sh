#!/usr/bin/env bash

MAIN=cdt2d.x
#Removes the executable (cdt2d.x) and data files in out/*.dat.
rm -f ${MAIN} out/*.dat

# Rebuilds the code using make in the parent directory.
make --no-print-directory -C ..

Copies the executable to the current directory.
cp ../${MAIN} .

#Runs the simulation with config.dat as input.
./${MAIN} config.dat
