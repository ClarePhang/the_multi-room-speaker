#!/bin/bash

while [[ 1 ]];do
    mplayer ./untitled.mp3 -novideo -channels 2 -srate 44100 -af format=s16le -ao pcm:file=/tmp/speakers
    sleep 3
done

