# TessSVG: Command-Line SVG Geometry Compiler

TessSVG is a high-performance CLI tool that converts SVG paths into triangulated 2D geometry. It bridges the gap between vector design tools (Inkscape, Adobe Illustrator) and game engines/physics solvers.
# 🛠 What it does

It parses SVG files, applies all nested transformations, and exports the resulting coordinates into various formats. It’s perfect for generating collision hitboxes, navmesh data, or static vertex buffers.

# 🚀 Key Features

  -  Format Agnostic: Export to JSON (packed or pretty), Lua, Java, or C++ (SFML-compatible).

  -  Bezier Rasterization: Granular control over curve smoothness via --bparts.

  -  Smart Tessellation: Handles holes, self-intersections, and complex nested groups.

  -  Physics-Ready: Automatically resolves complex SVG <path> data into simple primitive arrays.
  
# 📥 Installation & Usage

## Basic Command

```
./tess_svg --help

./tess_svg -i sprite1.svg -o sprite1_polygon_data.json -J
```

## ⚠️ Critical Note: Orientation

Warning! Source SVG files must be designed assuming that the initial zero rotation (0°) of the Actor points to the right (--->).

# How it works

1. Parsing: Uses pugixml to traverse the SVG DOM.

2. Transforming: Recursively applies matrix, scale, rotate, and translate using Boost.uBLAS.

3. Tessellating: Employs GLU Tesselator to ensure polygons are manifold and hole-free.

4. Exporting: Formats the resulting vertex arrays into the requested language/format.
