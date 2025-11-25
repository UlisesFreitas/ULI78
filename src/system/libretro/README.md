# ULI-78 libretro

Provided is a [ULI-78](https://uli.computer) libretro core to render ULI-78 cartridges through the [libretro](https://www.libretro.com) API and [RetroArch](https://www.retroarch.com).

## Build

To build the core by itself, run the following commands.

```
git clone https://github.com/uli78/ULI-78.git
cd ULI-78
git submodule update --init --recursive
cd build
cmake .. -DBUILD_PLAYER=OFF -DBUILD_SDL=OFF -DBUILD_TOOLS=OFF -DBUILD_LIBRETRO=ON
make
```

## Usage

```
retroarch -L lib/uli78_libretro.so sfx.uli
```
