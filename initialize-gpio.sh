#!/bin/bash
ENCODE_UP=2
ENCODE_DOWN=3
TFT_ALL=18
TFT_NEXT=27
TFT_UPDOWN=22
FB_CS=23

gpio -g mode $ENCODE_UP input
gpio -g mode $ENCODE_DOWN input
gpio -g mode $FB_CS input

gpio -g mode $TFT_ALL output
gpio -g mode $TFT_NEXT output
gpio -g mode $TFT_UPDOWN output

gpio write $ADDRESS_ALL 1