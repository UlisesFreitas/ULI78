#!/bin/sh
make clean
make
uli78 --fs . --cmd 'load wasmdemo.wasmp & import binary cart.wasm & run & exit'
