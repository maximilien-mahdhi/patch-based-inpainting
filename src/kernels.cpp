#include "kernels.hpp"

bool build_kernel(const std::string& kernel_name, cv::Mat& kernel) {
    if (kernel_name == "sobelX") {
        kernel.at<double>(0, 0) = -1;
        kernel.at<double>(1, 0) = -2;
        kernel.at<double>(2, 0) = -1;
        kernel.at<double>(0, 2) = 1;
        kernel.at<double>(1, 2) = 2;
        kernel.at<double>(2, 2) = 1;
    }
    else if (kernel_name == "sobelY") {
        kernel.at<double>(0, 0) = -1;
        kernel.at<double>(0, 1) = -2;
        kernel.at<double>(0, 2) = -1;
        kernel.at<double>(2, 0) = 1;
        kernel.at<double>(2, 1) = 2;
        kernel.at<double>(2, 2) = 1;
    }
    else if (kernel_name == "prewittX") {
        kernel.at<double>(0, 0) = -1;
        kernel.at<double>(1, 0) = -1;
        kernel.at<double>(2, 0) = -1;
        kernel.at<double>(0, 2) = 1;
        kernel.at<double>(1, 2) = 1;
        kernel.at<double>(2, 2) = 1;
    }
    else if (kernel_name == "prewittY") {
        kernel.at<double>(0, 0) = -1;
        kernel.at<double>(0, 1) = -1;
        kernel.at<double>(0, 2) = -1;
        kernel.at<double>(2, 0) = 1;
        kernel.at<double>(2, 1) = 1;
        kernel.at<double>(2, 2) = 1;
    }
    else if (kernel_name == "laplacian1") {
        kernel.at<double>(0, 1) = -1;
        kernel.at<double>(1, 0) = -1;
        kernel.at<double>(1, 1) = 4;
        kernel.at<double>(1, 2) = -1;
        kernel.at<double>(2, 1) = -1;
    }
    else if (kernel_name == "laplacian2") {
        kernel.at<double>(0, 0) = 1;
        kernel.at<double>(0, 1) = 1;
        kernel.at<double>(0, 2) = 1;
        kernel.at<double>(1, 0) = 1;
        kernel.at<double>(1, 1) = -8;
        kernel.at<double>(1, 2) = 1;
        kernel.at<double>(2, 0) = 1;
        kernel.at<double>(2, 1) = 1;
        kernel.at<double>(2, 2) = 1;
    }
    else if (kernel_name == "box_blur") {
        kernel.setTo(1);
        kernel /= 9; // sum(kernel) == 9 before normalization
    }
    else if (kernel_name == "gaussian") {
        kernel.at<double>(0, 0) = 1;
        kernel.at<double>(0, 1) = 2;
        kernel.at<double>(0, 2) = 1;
        kernel.at<double>(1, 0) = 2;
        kernel.at<double>(1, 1) = 4;
        kernel.at<double>(1, 2) = 2;
        kernel.at<double>(2, 0) = 1;
        kernel.at<double>(2, 1) = 2;
        kernel.at<double>(2, 2) = 1;
        kernel /= 16; // sum(kernel) == 16 before normalization
    }
    else {
        return false;
    }
    return true;
}