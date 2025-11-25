# Package

version       = "0.1.0"
author        = "archargelod"
description   = "uli78 wasm template for Nim language"
license       = "MIT"
srcDir        = "src"


# Dependencies

requires "nim >= 2.0.0"

# Tasks
import std/strformat

let pwd = getCurrentDir()

task wasmbuild, "Build wasm binary (debug)":
  exec("nim c -o:cart.wasm src/cart")

task wasmrelease, "Build wasm binary (release)":
  exec("nim c -d:release -o:cart.wasm src/cart")

task buildcart, "Build optimized wasm binary and import it to uli78 cart":
  rmFile("cart.uli")
  exec("nim c -d:release -o:cart.wasm src/cart")
  exec(&"uli78 --cli --fs=\"{pwd}\" --cmd=\"load src/cart.uli & import binary cart.wasm & save cart.uli & exit\"")

task runcart, "Build optimized wasm binary and run it with uli78":
  rmFile("cart.uli")
  exec("nim c -d:release -o:cart.wasm src/cart")
  exec(&"uli78 --skip --fs=\"{pwd}\" --cmd=\"load src/cart.uli & import binary cart.wasm & save cart.uli & run\"")

task debugcart, "Build wasm binary in debug mode and run it with uli78":
  rmFile("cart.uli")
  exec("nim c -o:cart.wasm src/cart")
  exec(&"uli78 --skip --fs=\"{pwd}\" --cmd=\"load src/cart.uli & import binary cart.wasm & save cart.uli & run\"")
