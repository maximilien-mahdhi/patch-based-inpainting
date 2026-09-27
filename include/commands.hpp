#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <string>

/**
 * @brief Prints the list of available command-line flags.
 */
void print_usage();

/**
 * @brief Handles the -i flag: parses polygon coordinates and runs region-filling inpainting.
 *        If no coordinates are given on the command line, falls back to interactive
 *        mouse selection.
 */
void run_inpainting(const std::string& file_name, int argc, char* argv[]);

/**
 * @brief Handles the -r and -er flags: loads the image and applies the requested rotation.
 * @param elastic If true, applies elasticity_rotation instead of a rigid rotation.
 */
void run_rotation(const std::string& file_name, int argc, char* argv[], bool elastic);

/**
 * @brief Handles the -conv flag: builds the requested kernel and convolves the image with it.
 */
void run_convolution(const std::string& file_name, int argc, char* argv[]);

#endif