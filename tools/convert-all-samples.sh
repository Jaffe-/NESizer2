#!/bin/bash


SAMPLEDIR=$1
# enter DIRECTORY containing all samples for conversion, without trailing slash
# e.g. C:/Users/me/Desktop/samples

STARTINDEX=$2
# enter start index (0-99), all others will be assigned in order

# if you sort your samples with a prefix number in the file name, and there are more than 10,
# start the first 9 with leading 0's so bash will index them correctly.
# e.g. 01-kick.wav, 02-snare.wav, ... , 10-cowbell.wav, etc.

i=$STARTINDEX
for filename in $SAMPLEDIR/*.wav; do
    # echo $filename
    # echo $i
    sox $filename -r 16000 -b 8 -e unsigned-integer $filename.raw
    ./gensysex sample $i $filename.raw $filename.syx
    rm $filename.raw
    i=$(($i+1))
done
