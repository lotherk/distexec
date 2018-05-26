#!/bin/bash
read LINE
echo "r1 " $LINE
echo "stderr error" >> /dev/stderr
read R3
echo "r3 " $R3

