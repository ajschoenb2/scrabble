#!/bin/sh

CC_OPT=-O3 BUILD_DIR=build make clean all
./build/scrabble
