#!/usr/bin/env sh

# TODO: Stop hardcoding this.
alias emcc=~/code/emsdk/upstream/emscripten/emcc
emcc -O2 src/wasm.cpp -o build/bbpPairing.html
