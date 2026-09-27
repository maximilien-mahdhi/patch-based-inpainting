#ifndef INTERACTIVE_SELECTION_HPP
#define INTERACTIVE_SELECTION_HPP

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

/**
 * @brief Lets the user select a polygon by clicking on the image with the mouse.
 *        Segments alternate horizontal/vertical, matching the -i coordinate format.
 * @param file_name Path to the image to display.
 * @return The selected coordinates in the same format as the -i command-line flag,
 *         or an empty vector if the selection was cancelled or invalid.
 */
std::vector<unsigned int> select_polygon_interactive(const std::string& file_name);

#endif