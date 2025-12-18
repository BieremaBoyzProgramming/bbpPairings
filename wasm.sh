#!/usr/bin/env sh

# TODO: Use the Makefile instead.

# TODO: Stop hardcoding this.
alias emcc=~/code/emsdk/upstream/emscripten/emcc
# TODO: Remove -g and ASSERTIONS=1 for production build.
emcc -lembind -O2 -o build/bbpPairing.html -I./src src/wasm.cpp src/*/*.cpp src/*/*/*.cpp -s ASSERTIONS=1

# TODO: Better JS API using:
# -s MODULARIZE=1 
# -s EXPORT_NAME="MyPairingModule"
