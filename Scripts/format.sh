#!/usr/bin/env bash

FILES=$(find Source \
	-type f \( \
	-name '*.cpp' -o \
	-name '*.c' -o \
	-name '*.h' -o \
	-name '*.hpp' \
	\))

clang-format -i $FILES
