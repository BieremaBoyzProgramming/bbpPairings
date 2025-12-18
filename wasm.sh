#!/usr/bin/env sh

# TODO: Stop hardcoding this.
alias emcc=~/code/emsdk/upstream/emscripten/emcc
emcc -lembind -O2 -o build/bbpPairing.html src/wasm.cpp

# TODO: Better JS API using:
# -s MODULARIZE=1 
# -s EXPORT_NAME="MyPairingModule"
