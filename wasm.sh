#!/usr/bin/env sh

# TODO: Use the Makefile instead.
emcc -lembind \
  -O2 \
  -o build/bbpPairing.js \
  -I./src \
  src/wasm.cpp src/*/*.cpp src/*/*/*.cpp \
  -s NO_DISABLE_EXCEPTION_CATCHING=1 \
  -s ASSERTIONS=1 \
  -s MODULARIZE=1 \
  -s EXPORT_ES6=1 \
  -s EXPORT_NAME="BbpPairingModule"
