#!/bin/bash
PROG=/usr/local/bin/gpio

ENCODE_UP=2
ENCODE_DOWN=3
TFT_ALL=18
TFT_NEXT=27
TFT_UPDOWN=22
FB_CS=23

${PROG} -g mode $ENCODE_UP input
${PROG} -g mode $ENCODE_DOWN input
${PROG} -g mode $FB_CS input

${PROG} -g mode $TFT_ALL output
${PROG} -g mode $TFT_NEXT output
${PROG} -g mode $TFT_UPDOWN output

${PROG} write $TFT_ALL 1