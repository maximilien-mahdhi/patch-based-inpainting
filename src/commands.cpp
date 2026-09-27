#include "commands.hpp"
#include "kernels.hpp"
#include "interactive_selection.hpp"
#include "logger.hpp"
#include "image.hpp"
#include "config.hpp"

#include <opencv2/opencv.hpp>
#include <iostream>
#include <sstream>
#include <chrono>

void print_usage() {
    // Plain help text, not a log message: stays on std::cout directly
    std::cout << "Usage: ./main <flag> <image_path> [parameters]\n";
    std::cout << "Available flags:\n";
    std::cout << "  -help  : prints this command flag list\n";
    std::cout << "  -i     : region-filling inpainting (polygon domain). No coords -> interactive mouse selection\n";
    std::cout << "  -conv  : convolve with a named kernel\n";
    std::cout << "  -r     : rigid rotation\n";
    std::cout << "  -er    : elasticity (distance-weighted) rotation\n";
}

void run_inpainting(const std::string& file_name, int argc, char* argv[]) {
    std::vector<unsigned int> coords;

    if (argc == 3) {
        // No coordinates passed: select the polygon interactively.
        // select_polygon_interactive already checks the image loads correctly
        // and logs "Image read successfully!" before the preview is shown.
        coords = select_polygon_interactive(file_name);
        if (coords.empty()) return;  // cancelled selection, or the image failed to load
    } else {
        // Coordinates are given as a compact rectangular sequence: each new value
        // alternates between completing a horizontal edge and a vertical edge.
        coords.push_back(atoi(argv[3]));
        coords.push_back(atoi(argv[4]));

        bool horizontal = true;
        for (int i = 5; i < argc; i++) {
            if (horizontal) {
                coords.push_back(coords[coords.size() - 2]);
                coords.push_back(atoi(argv[i]));
            } else {
                coords.push_back(atoi(argv[i]));
                coords.push_back(coords[coords.size() - 2]);
            }
            horizontal = !horizontal;
        }
    }

    image img(file_name);
    if (img.rows() == 0 || img.cols() == 0) {
        Logger::error("Unable to load the image: " + file_name);
        return;
    }
    Logger::info("Image read successfully!");

    if (!img.imwrite(std::string(OUTPUT_DIR) + "/before_recovering.png")) {
        Logger::error("Failed to save the image to " + std::string(OUTPUT_DIR) + "/before_recovering.png");
    }

    auto start = std::chrono::high_resolution_clock::now();
    img.region_filling_recovery(coords);
    auto stop = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::ostringstream oss;
    oss << "Process time of recovery: " << duration.count() << " ms";
    Logger::info(oss.str());
}

void run_rotation(const std::string& file_name, int argc, char* argv[], bool elastic) {
    if (argc != 4) {
        Logger::error("Not the right amount of parameters");
        return;
    }

    image img(file_name);
    if (img.rows() == 0 || img.cols() == 0) {
        Logger::error("Unable to load the image: " + file_name);
        return;
    }
    Logger::info("Image read successfully!");

    float angle = static_cast<float>(atof(argv[3]));

    if (elastic) {
        img.elasity_rotation(angle);
    } else {
        img.rotation(angle);
    }

    if (!img.imwrite(std::string(OUTPUT_DIR) + "/rotated_image.png")) {
        Logger::error("Failed to save the image to " + std::string(OUTPUT_DIR) + "/rotated_image.png");
    }
}

void run_convolution(const std::string& file_name, int argc, char* argv[]) {
    if (argc < 4) {
        Logger::error("The kernel name is missing as an argument for the convolution");
        return;
    }

    std::string kernel_name = argv[3];
    cv::Mat kernel = cv::Mat::zeros(3, 3, CV_64FC1);

    if (!build_kernel(kernel_name, kernel)) {
        Logger::error("No kernel named \"" + kernel_name + "\" for convolution");
        return;
    }

    cv::Mat raw = cv::imread(file_name);
    if (raw.empty()) {
        Logger::error("Unable to load the image: " + file_name);
        return;
    }
    Logger::info("Image read successfully!");

    image img(raw);
    if (!img.imwrite(std::string(OUTPUT_DIR) + "/before_convolving.png")) {
        Logger::error("Failed to save the image to " + std::string(OUTPUT_DIR) + "/before_convolving.png");
    }

    auto start = std::chrono::high_resolution_clock::now();
    std::string output_path = std::string(OUTPUT_DIR) + "/conv_" + kernel_name + ".png";
    if (!img.convolve(kernel).imwrite(output_path)) {
        Logger::error("Failed to save the image to " + output_path);
    }
    auto stop = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::ostringstream oss;
    oss << "Process time of convolution: " << duration.count() << " ms";
    Logger::info(oss.str());
}