#include <iostream>
#include <string>
#include <filesystem>

#include "commands.hpp"
#include "logger.hpp"
#include "config.hpp"

// ============================================================================
// USAGE EXAMPLES
//
//  ROTATION
//    Syntax: ./build/main -r|-er <image_path> <angle>
//    ./build/main -er image_to_rotate.jpeg 60
//    ./build/main -r image_to_rotate.jpeg 60
//
//  INPAINTING
//    Syntax: ./build/main -i <image_path> [r1 c1 r2 c2 ...]
//    -> if no coordinates are given, opens an interactive window to select
//       the polygon with the mouse instead
//    ./build/main -i beach.png                                                       (interactive mouse selection)
//    ./build/main -i clean_finger_small.png 73 49 72 143 49
//    ./build/main -i beach.png 121 169 231 102 258 246 277 305 335 329 190 220 169   (long computation)
//    ./build/main -i beach.png 125 172 210 151 223 102 258 154 242 169 235 245 276 289 242 307 336 330 216 271 195 221 161 165 172   (long computation)
//
//  CONVOLUTION
//    Syntax: ./build/main -conv <image_path> <kernel_name>
//    ./build/main -conv image.png <kernel_name>
//
// GENERAL SYNTAX:
//    ./build/main <flag> <image_path> [flag-specific parameters]
//
// AVAILABLE KERNELS FOR -conv:
//
//  Edge detection:
//    "sobelX"     : Sobel operator, x direction
//    "sobelY"     : Sobel operator, y direction
//    "prewittX"   : Prewitt operator, x direction
//    "prewittY"   : Prewitt operator, y direction
//    "laplacian1" : gives more weight to the center pixel
//    "laplacian2" : gives more weight to the edges, strong negative center contribution
//
//  Blurring:
//    "box_blur"   : equal contribution from every pixel in the neighborhood
//    "gaussian"   : Gaussian-weighted kernel
// ============================================================================

int main(int argc, char* argv[]) {
    std::filesystem::create_directories(OUTPUT_DIR);  // ensures the output folder exists before any imwrite call

    if (argc < 2) {
        Logger::error("Not enough parameters");
        print_usage();
        return 1;
    }

    std::string flag = argv[1];

    if (flag == "-help") {
        print_usage();
        return 0;
    }

    if (argc < 3) {
        Logger::error("Not enough parameters");
        print_usage();
        return 1;
    }

    std::string file_name = argv[2];

    if (flag == "-i") {
        run_inpainting(file_name, argc, argv);
    }
    else if (flag == "-er") {
        run_rotation(file_name, argc, argv, /* elastic = */ true);
    }
    else if (flag == "-r") {
        run_rotation(file_name, argc, argv, /* elastic = */ false);
    }
    else if (flag == "-conv") {
        run_convolution(file_name, argc, argv);
    }
    else {
        Logger::error("Unknown flag: " + flag);
        print_usage();
        return 1;
    }

    return 0;
}