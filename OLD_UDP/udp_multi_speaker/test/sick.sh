#!/bin/bash

mplayer ./sick.mp3 -novideo -channels 2 -srate 44100 -af format=s16le -ao pcm:file=/tmp/speakers
