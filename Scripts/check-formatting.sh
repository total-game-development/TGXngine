#!/usr/bin/env bash
set -euo pipefail

FILES=$(find Source \
	-type f \( \
	-name '*.cpp' -o \
	-name '*.c' -o \
	-name '*.h' -o \
	-name '*.hpp' \
	\))

echo "Checking clang-format on:"
echo "$FILES"

clang-format --dry-run -Werror $FILES
