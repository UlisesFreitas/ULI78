#!/bin/sh
rm -f game.uli
make clean
make
uli78 --fs . --cmd 'load wasmdemo.wasmp & import binary cart.wasm & save game.uli & exit'
uli78 --fs . --cmd 'load game.uli & run & exit'
