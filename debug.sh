#!/bin/sh

git pull
CC_OPT=-O0 BUILD_DIR=build make clean all
gdb build/scrabble
