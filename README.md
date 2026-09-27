# Patch-Based Inpainting

![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B&logoColor=white)
![OpenCV](https://img.shields.io/badge/OpenCV-4.x-5C3EE8?logo=opencv&logoColor=white)
![OpenMP](https://img.shields.io/badge/OpenMP-parallelized-informational)
![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux-lightgrey)

*Last update : 27/09/2026*

## Author

*Maximilien MAHDHI — 31/01/2025*

## Overview

A C++/OpenCV implementation of exemplar-based image inpainting, following the
patch-priority algorithm of Criminisi, Pérez and Toyama (2004), along with
a small set of complementary image-processing tools (rotation, convolution).

This work stems from a 2-month project in image processing done in December 2024
during my M1 MSIAM at UGA/Ensimag. The code is not perfect.

## Features

- **Region-filling inpainting** — removes a user-defined region from an image
  and reconstructs it by propagating texture and structure from the rest of
  the image, patch by patch, prioritizing patches along strong edges first.
- **Interactive region selection** — select the region to inpaint directly
  with the mouse instead of typing pixel coordinates.
- **Image rotation** — rigid rotation, and an experimental elastic
  (distance-weighted) rotation.
- **Convolution** — apply named kernels (Sobel, Prewitt, Laplacian, box blur,
  Gaussian) to an image.
- **Parallelized with OpenMP** — the core patch-search routine, the main
  computational bottleneck, is parallelized for multi-core CPUs.

## Example

| Original : `fingerprint_hole.png` | Region to remove | Result : `region_filling_output.png` |
|---|---|---|
| ![original](test_images/fingerprint_hole.png) | *(selected interactively or via coordinates)* | ![result](output_images/region_filling_output.png) |

*This example can be reproduced by running :*
```
./build/main -i test_images/fingerprint_hole.png 73 49 72 143 49
```

## Requirements

- A C++17-compatible compiler (for `<filesystem>`)
- [OpenCV](https://opencv.org/) (core, imgproc, highgui modules)
- OpenMP (bundled with GCC; on macOS, install via `brew install libomp`)
- `make`

## Build

```bash
make
```

This produces the executable at `build/main`.

## Usage

### General syntax

```
./build/main <flag> <image_path> [flag-specific parameters]
```

```
./build/main -help
```

```
Usage: ./build/main <flag> <image_path> [parameters]
Available flags:
  -help  : prints this command flag list
  -i     : region-filling inpainting (polygon domain). No coords -> interactive mouse selection
  -conv  : convolve with a named kernel
  -r     : rigid rotation
  -er    : elasticity (distance-weighted) rotation
```

### Flags at a glance

| Flag | Purpose | Parameters | Example |
|---|---|---|---|
| `-help` | Show the flag list | *(none)* | `./build/main -help` |
| `-i` | Region-filling inpainting | `[r1 c1 r2 c2 ...]` (optional — omit for interactive mouse selection) | `./build/main -i beach.png` |
| `-r` | Rigid rotation | `<angle>` (degrees) | `./build/main -r beach.png 60` |
| `-er` | Elastic (distance-weighted) rotation | `<angle>` (degrees) | `./build/main -er beach.png 60` |
| `-conv` | Convolve with a named kernel | `<kernel_name>` | `./build/main -conv beach.png sobelX` |

---

### Inpainting (`-i`)

**Interactive selection (recommended):**

```bash
./build/main -i test_images/fingerprint_hole.png
```

Click the polygon vertices directly on the image. Segments alternate between
horizontal and vertical edges, and the last point must line up with the first
one's column to close the polygon. Live visual feedback shows the guide line
and the closing edge (green when valid, orange otherwise).

| Key | Action |
|---|---|
| `Enter` | Confirm the selection and run the inpainting |
| `Backspace` | Undo the last point |
| `Escape` | Cancel the selection |

**Manual coordinates:**

```bash
./build/main -i test_images/fingerprint_hole.png 73 49 72 143 49
./build/main -i test_images/beach.png 121 169 231 102 258 246 277 305 335 329 190 220 169
```

Coordinates are given as `r1 c1 r2 c2 ...` (row, column pairs), alternating
horizontal and vertical edges, following the same closing convention as the
interactive mode: the polygon's last point must share its column with the
first point, since the closing edge is always drawn vertical.

> ⚠️ Inpainting a large image (e.g. `beach.png`) can take a while — the patch
> search is the dominant cost. See the [Performance](#performance) section.

**What gets generated:** `before_recovering.png`, `initial_image.png`,
`image_hole.png`, `confidence_image.png`, and the final
`region_filling_output.png` (plus `selection_preview.png` if the interactive
mode was used) — all under `output_images/`.

### Rotation (`-r` / `-er`)

```bash
./build/main -r image_to_rotate.jpeg 60    # rigid rotation, 60°
./build/main -er image_to_rotate.jpeg 60   # elastic rotation, 60°
```

- `-r` applies a standard rigid rotation around the image center
  (`cv::warpAffine`, no holes).
- `-er` applies a distance-weighted "elastic" rotation: the rotation
  strength decreases with distance from a fixed center point, producing a
  swirl-like effect rather than a uniform rotation.

**What gets generated:** `rotated_image.png` under `output_images/`.

### Convolution (`-conv`)

```bash
./build/main -conv image.png sobelX
```

| Category | Kernels |
|---|---|
| Edge detection | `sobelX`, `sobelY`, `prewittX`, `prewittY`, `laplacian1`, `laplacian2` |
| Blurring | `box_blur`, `gaussian` |

**What gets generated:** `before_convolving.png` and
`conv_<kernel_name>.png` under `output_images/`.

## Output

All generated images are written to `output_images/`, created automatically
on first run.

## Project structure

```
patch-based-inpainting/
├── apps/
│   └── main.cpp                        — CLI argument parsing and dispatch
├── include/
│   ├── image.hpp                       — image wrapper around cv::Mat, core algorithms
│   ├── map.hpp                         — pixel confidence/priority map used during inpainting
│   ├── pixel.hpp                       — per-pixel state (color, confidence)
│   ├── commands.hpp                    — implementation of each CLI flag
│   ├── kernels.hpp                     — convolution kernel definitions
│   ├── interactive_selection.hpp       — mouse-based polygon selection
│   ├── logger.hpp                      — leveled, colored console logging
│   └── config.hpp                      — shared constants (output directory, ...)
├── src/
│   ├── image.cpp
│   ├── map.cpp
│   ├── pixel.cpp
│   ├── commands.cpp
│   ├── kernels.cpp
│   ├── interactive_selection.cpp
│   └── logger.cpp
├── Images/                             — sample input images
├── output_images/                      — generated output (created automatically)
├── bib/                                — reference papers
├── Makefile
└── README.md
```

## Algorithm

The inpainting method follows the exemplar-based approach described in:

> A. Criminisi, P. Pérez, K. Toyama, *"Region Filling and Object Removal by
> Exemplar-Based Image Inpainting"*, IEEE Transactions on Image Processing,
> 2004. (see `bib/criminisi_tip2004.pdf`)

In short: the region to fill is bounded by a "front line" of border pixels,
each assigned a priority based on local confidence and isophote strength.
At each step, the highest-priority patch is filled by copying the most
similar patch found elsewhere in the image, and the front line is updated
accordingly.

## Performance

The dominant cost is `nearest_patch`, which searches the whole image for the
best-matching patch at every step of the filling process. It is parallelized
with OpenMP (manual argmin reduction across threads).