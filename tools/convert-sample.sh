#!/bin/bash

INFILE=$1
INDEX=$2

sox $INFILE -r 16000 -b 8 -e unsigned-integer $INFILE.raw
./gensysex sample $INDEX $INFILE.raw $INFILE.syx
rm $INFILE.raw
