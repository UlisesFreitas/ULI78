**Preview Ctrl + Shift + V**

[![Build Status](https://github.com/uli78/ULI78/workflows/Build/badge.svg)](https://github.com/uli78/ULI78/actions?query=workflow%3ABuild)

![ULI78](https://uli78.com/img/logo64.png)
**ULI78 TINY COMPUTER** — [uli78.com](https://uli78.com)

- [About](#about)
  - [Features](#features)
- [Pro Version](#pro-version)
  - [Pro Features](#pro-features)
- [Build Instructions](#build-instructions)
  - [Windows](#windows)
      - [Windows 10 / 11 64-bit (x64)](#windows-10--11-64-bit-x64)
  - [Credits](#credits)

# About
ULI78 is a free and open source fantasy computer for making, playing and sharing tiny games.

With ULI78 you get built-in tools for development: code, sprites, maps, sound editors and the command line, which is enough to create a mini retro game.

Games are packaged into a cartridge file, which can be easily distributed. ULI78 works on all popular platforms. This means your cartridge can be played in any device.

To make a retro styled game, the whole process of creation and execution takes place under some technical limitations: 240x136 pixel display, 16 color palette, 256 8x8 color sprites, 4 channel sound, etc.

![ULI78](https://user-images.githubusercontent.com/1101448/92492270-d6bcbc80-f1fb-11ea-9d2d-468ad015ace2.gif)

### Features
- Programming languages: [Lua](https://www.lua.org)
- Games can have mouse and keyboard as input
- Games can have up to 4 controllers as input (with up to 16 buttons, each)
- Built-in editors: for code, sprites, world maps, sound effects and music
- An additional memory bank: load different assets from your cartridge while your game is executing

# Extended buttons for gamepad and keyboard
| ACTION | P1 | P2 | P3 | P4 |
| :---: | :---: | :---: | :---: | :---: |
| UP | 0 | 16 | 32 | 48 |
| DOWN | 1 | 17 | 33 | 49 |
| LEFT | 2 | 18 | 34 | 50 |
| RIGHT | 3 | 19 | 35 | 51 |
| A | 4 | 20 | 36 | 52 |
| B | 5 | 21 | 37 | 53 |
| X | 6 | 22 | 38 | 54 |
| Y | 7 | 23 | 39 | 55 |
| START(V) | 8 | 24 | 40 | 56 |
| SELECT(C) | 9 | 25 | 41 | 57 |
| L1/LB(W) | 10 | 26 | 42 | 58 |
| R1/RB(E) | 11 | 27 | 43 | 59 |
| L2/LT(Q) | 12 | 28 | 44 | 60 |
| R2/RT(R) | 13 | 29 | 45 | 61 |
| GUIDE(G) | 14 | 30 | 46 | 62 |

# Binary Downloads

## Stable Builds
You can download compiled versions for the major operating systems directly from our [Releases](https://github.com/uli78/ULI78/releases) page.

# Pro Version
To help support ULI78 development, we have a [PRO Version](https://uli78.itch.io/uli78).

This version has a few additional features and binaries can only be downloaded on our itch.io page.

For users who can't afford the program can easily build the pro version from the source code using `cmake .. -DBUILD_PRO=On` command.

## Pro Features
- Save/load cartridges in text format, and create your game in any editor you want, also useful for version control systems.
- Even more memory banks: instead of having only 1 memory bank you have 8.
- Export your game without editors, and then publish it to app stores.

# Build instructions

## Windows

#### Windows 10 / 11 64-bit (x64)
This guide assumes you're running PowerShell with an elevated prompt.

- Install [Git](https://git-scm.com/download/win), [CMake](https://cmake.org/download), [Visual Studio 2019 Build Tools](https://winstall.app/apps/Microsoft.VisualStudio.2019.BuildTools) and [Ruby+Devkit 2.7.8 x64](https://github.com/oneclick/rubyinstaller2/releases/download/RubyInstaller-2.7.8-1/rubyinstaller-devkit-2.7.8-1-x64.exe) manually or with [WinGet](https://github.com/microsoft/winget-cli):
```
winget install Git.Git Kitware.CMake Microsoft.VisualStudio.2019.BuildTools RubyInstallerTeam.RubyWithDevKit.2.7
```
- Install the neccessary dependencies within VS2019:
  - Launch "Visual Studio Installer"
  - Click "Modify"
  - Check "Desktop Development with C++"
  - Make sure the following components are installed:
    - Windows 10 SDK (10.0.19041.0)
    - MSVC v142 - VS 2019 C+ + x64/x86 build tools (Latest)
  - Click "Modify"
- Run `ridk install` with options `1,3` to set up [MSYS2](https://www.msys2.org) and development toolchain
- Add MSYS2's [`gcc`](https://gcc.gnu.org) at `C:\Ruby27-x64\msys64\mingw64\bin` to your `$PATH` [manually](https://www.java.com/en/download/help/path.html#:~:text=Mac%20OS%20X.-,Windows,-Windows%2010%20and) or with the following PowerShell command:

```
[Environment]::SetEnvironmentVariable('Path', $env:Path + ';C:\Ruby27-x64\msys64\mingw64\bin', [EnvironmentVariableTarget]::Machine)
```

- Open a new elevated prompt and run the following commands:

```
git clone --recursive https://github.com/uli78/ULI78
```
```
cd .\ULI78\build
```
```
cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_SDLGPU=On -DBUILD_WITH_ALL=On ..
```
```
cmake --build . --parallel
```

You'll find `uli78.exe` in `ULI78\build\bin`.

## Credits
* Filippo Rivato — [Twitter @HomineLudens](https://twitter.com/HomineLudens)
* Fred Bednarski — [Twitter @FredBednarski](https://twitter.com/FredBednarski)
* Al Rado — [Twitter @alrado2](https://twitter.com/alrado2)
* Trevor Martin — [Twitter @trelemar](https://twitter.com/trelemar)
* MonstersGoBoom — [Twitter @MonstersGoBoom](https://twitter.com/MonstersGo)
* Matheus Lessa — [Twitter @matheuslrod](https://twitter.com/matheuslrod)
* CliffsDover — [Twitter @DancingBottle](https://twitter.com/DancingBottle)
* Frantisek Jahoda — [GitHub @jahodfra](https://github.com/jahodfra)
* Guilherme Medeiros — [GitHub @frenetic](https://github.com/frenetic)
* Andrei Rudenko — [GitHub @RudenkoArts](https://github.com/RudenkoArts)
* Phil Hagelberg — [@technomancy](https://technomancy.us/colophon)
* Rob Loach — [Twitter @RobLoach](https://twitter.com/RobLoach) [GitHub @RobLoach](https://github.com/RobLoach)
* Wade Brainerd — [GitHub @wadetb](https://github.com/wadetb)
* Paul Robinson — [GitHub @paul59](https://github.com/paul59)
* Stefan Devai — [GitHub @stefandevai](https://github.com/stefandevai) [Blog stefandevai.me](https://stefandevai.me)
* Damien de Lemeny — [GitHub @ddelemeny](https://github.com/ddelemeny)
* Adrian Siekierka — [GitHub @asiekierka](https://github.com/asiekierka) [Website](https://asie.pl/)
* Jay Em (Sweetie16 palette) — [Twitter @GrafxKid](https://twitter.com/GrafxKid)
* msx80 — [Twitter @msx80](https://twitter.com/msx80) [Github msx80](https://github.com/msx80)
* Josh Goebel — [Twitter @dreamer3](https://twitter.com/dreamer3) [Github joshgoebel](https://github.com/joshgoebel)
* Joshua Minor — [GitHub @jminor](https://github.com/jminor)
* Julia Nelz — [Github @remi6397](https://github.com/remi6397) [WWW](https://nelz.pl)
* Thorben Krüger — [Mastodon @benthor@chaos.social](https://chaos.social/@benthor)
* David St—Hilaire — [GitHub @sthilaid](https://github.com/sthilaid)
* Alec Troemel — [Github @alectroemel](https://github.com/AlecTroemel)
* Kolten Pearson — [Github @koltenpearson](https://github.com/koltenpearson)
* Cort Stratton — [Github @cdwfs](https://github.com/cdwfs)
* Alice — [Github @aliceisjustplaying](https://github.com/aliceisjustplaying)
* Sven Knebel — [Github @sknebel](https://github.com/sknebel)
* Graham Bates — [Github @grahambates](https://github.com/grahambates)
* Kii — [Github @kiikrindar](https://github.com/kiikrindar)
* Matt Westcott — [Github @gasman](https://github.com/gasman)
* NuSan — [Github @TheNuSan](https://github.com/thenusan)
* Li Jin — [Github @pigpigyyy](https://github.com/pigpigyyy)
* Dania Rifki — [Github @Kaleidosium](https://github.com/Kaleidosium)
