# ZIG Starter Project Template

This is a ZIG / ULI-78 starter template. To build it, ensure you have the latest development release (`0.14.0-dev.1421+f87dd43c1` or newer), then run:

```
zig build --release=small
```

To import the resulting WASM to a cartridge:

```
uli78 --fs . --cmd 'load cart.wasmp & import binary zig-out/bin/cart.wasm & save'
```

Or from the ULI-78 console:

```
uli78 --fs .

load zig-out/bin/cart.wasmp
import binary cart.wasm
save
```

This is assuming you've run ULI-78 with `--fs .` inside your project directory.

Or easy call it :)
```zsh
sh run.sh
```
