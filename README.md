# Duke Nukem 3D - Nintendo DS Port

A port of Duke Nukem 3D's Build engine to the Nintendo DS, using devkitPro/devkitARM toolchain.

## Features

- Software-rendered Build engine adapted for NDS (256x192 framebuffer)
- Portal-based sector rendering with textured walls
- D-Pad + touch screen controls
- Debug console on bottom screen with FPS counter
- Procedural demo level (no copyrighted data required)
- Full Duke3D .MAP and .ART file loading support

## Building

### Requirements
- [devkitPro](https://devkitpro.org/) with devkitARM and libnds

### Local Build
```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM
make
```

### CI Build
Automated builds via GitHub Actions using the `devkitpro/devkitarm` Docker image.

## Controls

| Button | Action |
|--------|--------|
| D-Pad Up/Down | Move forward/backward |
| D-Pad Left/Right | Turn |
| L/R | Strafe |
| A | Fire |
| B | Jump |
| X | Use/Open |
| Y | Switch weapon |
| Touch screen | Look around |
| START | Start game |
| START+SELECT | Quit |

## Game Data

Place Duke Nukem 3D data files in the root of your flashcard's filesystem:
- `e1l1.map` ... `e4l8.map` (level files)
- `tiles000.art` (tile graphics)
- `palette.dat` (color palette)

Without game data, a procedural demo level will be generated.

## License

Based on the original Duke Nukem 3D source code released under GPL v2.
