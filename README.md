# YAE Level Converter

[English](README.md) · [Русский](README.ru.md)

A command-line converter of *You Are Empty* (2006) levels and models — the DS2 and DS2MD formats —
to OBJ, written in C++17. It is the oldest tool of the family (first release 2013, Linux build
2026) and the lightest way to get a level's geometry, materials and model instances into
3ds Max, Maya or Blender. For skeletons, animations, round-trip writing of the formats and a
graphical editor see the [YAE SDK](https://github.com/OpenYAE/yae-sdk).

Part of [Project Empty](https://github.com/OpenYAE), the open ecosystem around the game.

## Building (Linux)

Requires CMake 3.15+ and a C++17 compiler. The `xray_re-tools` submodule provides the OBJ and
config code; its Linux compatibility patches are applied automatically at configure time.

```bash
git clone --recurse-submodules https://github.com/OpenYAE/yae-level-converter
cd yae-level-converter
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

A Visual Studio project (`level_converter.vcxproj`) is kept for Windows.

## Preparing the data

1. Set the paths in `fs_yae_converter.ltx`.
2. Unpack the textures into `gameres/textures/`.
3. Unpack the maps you need into `gameres/maps/`.
4. Unpack the models into `gameres/models/` (needed to export model instances).

Any directories will do; then edit the paths in `fs_yae_converter.ltx`. The game's resources are
not part of this repository: use the files of your own copy of the game.

## Usage

Levels:

```
yae_converter -level <level_folder> -out <output_path> [-mode <max|maya>] [-split] [-scale <factor>]
```

Models:

```
yae_converter -model <model_file> -obj|-object <output_path> [-mode <max|maya>]
```

| Parameter | Meaning |
|---|---|
| `-level <name>` | Name of the map folder (`med1`, `map00`, `map06`, …) |
| `-model <file>` | Path to the model file, with extension |
| `-out <path>` | Path of the output OBJ, without extension |
| `-mode <max\|maya>` | Which editor the OBJ is written for (default `max`) |
| `-split` | Split the geometry into one file per game material |
| `-scale <factor>` | Vertex scale (for example `-scale 0.01` to go from centimetres to metres) |
| `-obj\|-object` | Output format of the model's geometry |

## What is converted

- Level geometry (static and dynamic meshes) with correct transforms
- Texture coordinates and normals
- Materials (MTL with texture paths)
- Model instances, when the DS2MD files are available in `gameres/models/`

## History

**v0.7 (30 Mar 2026)** — Linux build through CMake; correct transform matrices for dynamic meshes
and their normals; model instances with transforms; the `-scale` parameter; fixed DS2 string
reading (`uint16` length prefix), the vertex-colour layout (16 bytes per vertex) and config
parsing on Linux (path separators, line endings).

**v0.6 (26 Mar 2013)** — logging to a file; fixed the duplicated extension on OBJ export and the
export of some meshes and some Instinct levels.

**v0.5 (19 Mar 2013)** — model export to `object`, without bones yet; fixed index computation.

**v0.4 (9 Mar 2013)** — model geometry conversion; splitting by material; fixed skipped meshes.

**v0.3 (3 Mar 2013)** — fixes to mesh conversion, geometry grouping and polygons dropped on import.

**v0.2 (1 Mar 2013)** — config file and command-line switches; fixed the MTL path and 3ds Max texturing.

**v0.1 (26 Feb 2013)** — first release.

Author: K.D. Testing: Deathdoor, БогДан, Scarabey, Blu2z.

## Legal

*You Are Empty* and its formats belong to their rights holders; the converter contains no game
assets. The converter's license is being chosen by the maintainers (MIT proposed); note that the
`xray_re-tools` submodule carries its own license, which the choice has to respect. Until a
`LICENSE` file appears, all rights are reserved.
