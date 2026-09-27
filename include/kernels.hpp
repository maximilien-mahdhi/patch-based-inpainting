#ifndef KERNELS_HPP
#define KERNELS_HPP

#include <opencv2/opencv.hpp>
#include <string>

/**
 * @brief Builds a 3x3 convolution kernel from its name.
 * @param kernel_name Name of the kernel (see usage examples in commands.hpp).
 * @param kernel Output kernel matrix, filled in place. Must be a 3x3 CV_64FC1 zero matrix on entry.
 * @return True if the kernel name was recognized and the kernel was built successfully.
 */
bool build_kernel(const std::string& kernel_name, cv::Mat& kernel);

#endif