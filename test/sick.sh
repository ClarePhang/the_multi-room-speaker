#!/bin/bash

while [[ 1 ]];do
    if [[ -p /tmp/speakers ]];then
        mplayer ./sick.mp3 -novideo -channels 2 -srate 44100 -af format=s16le -ao pcm:file=/tmp/speakers
    else
        echo "waiting for pipe"
        sleep 3
    fi
    sleep 1
done


