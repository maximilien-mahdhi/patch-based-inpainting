#include <iostream>
#include <opencv2/opencv.hpp>
#include <cstdlib>
#include <vector>
#include <omp.h>

#include "image.hpp"
#include "map.hpp"
#include "logger.hpp"
#include "config.hpp"

// CONSTRUCTORS

image::image() : data(250, 250, CV_8U, cv::Scalar(0)), nb_channels(1) {}

image::image(const char * link) : nb_channels(1) {
  data = cv::imread(link, cv::IMREAD_GRAYSCALE);
}

image::image(std::string link) : nb_channels(1) {
  data = cv::imread(link, cv::IMREAD_GRAYSCALE);
}

image::image(const char * link, const unsigned int &nb) : nb_channels(nb) {
  data = cv::imread(link);
  if (data.empty()) {
    Logger::error(std::string("Unable to load the image from ") + link);
    exit(EXIT_FAILURE);
  }
}

image::image(const cv::Mat &M) : data(M), nb_channels(M.channels()) {}

image::image(const image &I) : data(I.data), nb_channels(I.data.channels()) {}

// OPERATOR OVERLOADS

bool image::operator <=(const image &M) const {
    return (cv::sum(data)[0] <= cv::sum(M.data)[0]);
}

image image::operator =(const cv::Mat &M) {
  int M_type = M.type();
  int this_type = this->data.type();
  if (M_type != this_type) {
    M.convertTo(M, CV_MAT_DEPTH(this_type));
  }
  data = M;
  this->nb_channels = M.channels();
  return *this;
}

// REIMPLEMENTATION OF PREEXISTING METHODS FROM CLASS "Mat" (OpenCV)

int image::rows() const { return data.rows; }
image image::row(const unsigned int &i) const { return image(data.row(i)); }
int image::cols() const { return data.cols; }
image image::col(const unsigned int &i) const { return image(data.col(i)); }

// MAIN OPERATIONS — delegated to OpenCV built-ins (already SIMD/multi-threaded internally)

void image::symetry_X() {
    cv::flip(data, data, 0);
}

void image::symetry_Y() {
    cv::flip(data, data, 1);
}

void image::t() {
  data = data.t();
}

void image::t2() {
    // 180-degree rotation is equivalent to flipping both axes at once
    cv::flip(data, data, -1);
}

void image::normalize2d() {
    if (data.channels() == 1) data.convertTo(data, CV_64FC1);
    else data.convertTo(data, CV_64FC3);
    data /= 255.0;
}

void normalizeKernel(cv::Mat& kernel) {
    double sumVal = cv::sum(cv::abs(kernel))[0];
    if (sumVal != 0) {
        kernel /= sumVal;
    }
}

void image::stretch2d() {
    cv::normalize(data, data, 0, 255, cv::NORM_MINMAX);
    data.convertTo(data, CV_8UC3);
}

// Convolution: pointer-based access + OpenMP (kept for full control vs. filter2D as a black box)
image image::convolve(cv::Mat kernel) {
    normalize2d();
    kernel.convertTo(kernel, CV_64FC1);
    normalizeKernel(kernel);

    cv::Mat temp(data.rows, data.cols, CV_64FC3, cv::Scalar(0));
    image output(temp);

    int kH = kernel.rows / 2;
    int kW = kernel.cols / 2;
    int rows = data.rows;
    int cols = data.cols;

    #pragma omp parallel for schedule(static)
    for (int n = 0; n < rows; n++) {
        cv::Vec3d* out_row = output.data.ptr<cv::Vec3d>(n);
        for (int m = 0; m < cols; m++) {
            cv::Vec3d sum(0, 0, 0);
            for (int i = -kH; i <= kH; i++) {
                int x = std::min(std::max(n + i, 0), rows - 1);
                const cv::Vec3d* src_row = data.ptr<cv::Vec3d>(x);
                const double* ker_row = kernel.ptr<double>(i + kH);
                #pragma omp simd
                for (int j = -kW; j <= kW; j++) {
                    int y = std::min(std::max(m + j, 0), cols - 1);
                    sum += src_row[y] * ker_row[j + kW];
                }
            }
            out_row[m] = sum;
        }
    }

    output.stretch2d();
    return output;
}

// METHODS TO ALTER IMAGES

void image::set_color_rectangle(const unsigned int &color, const unsigned int &r1, const unsigned int &c1, const unsigned int &r2, const unsigned int &c2) {
  data.convertTo(data, CV_8U);
  #pragma omp parallel for collapse(2) schedule(static)
  for (int i = r1; i <= r2; i++) {
    for (int j = c1; j <= c2; j++) {
      data.at<uchar>(i, j) = color;
    }
  }
}

// MAIN ALGORITHM IMPLEMENTATIONS

void image::region_filling_recovery(const std::vector<unsigned int>& coords) const {
  if (nb_channels == 1)
  {
    Logger::debug("Processing a gray image");

    std::string initial_path = std::string(OUTPUT_DIR) + "/initial_image.png";
    if (!(*this).imwrite(initial_path)) {
        Logger::error("Failed to save the image to " + initial_path);
    }

    map image_map(*this, coords);
    std::vector<std::tuple<std::tuple<unsigned int, unsigned int>, double>> frontline = image_map.Fill_Front(coords);

    Logger::debug("Image size: " + std::to_string(rows()) + "x" + std::to_string(cols()));

    cv::Mat white_image(rows(), cols(), CV_8UC1);
    #pragma omp parallel for collapse(2) schedule(static)
    for (int i = 0; i < rows(); i++) {
        for (int j = 0; j < cols(); j++) {
            white_image.at<uchar>(i, j) = image_map.P[i][j].color_value;
        }
    }
    white_image.at<uchar>(rows() - 1, 0) = 0;

    std::string hole_path = std::string(OUTPUT_DIR) + "/image_hole.png";
    if (!cv::imwrite(hole_path, white_image)) {
        Logger::error("Failed to save the image to " + hole_path);
    }

    // Main loop: strict sequential dependency (each iteration depends on the
    // result of the previous one) — not parallelizable, left as is.
    while (!frontline.empty()) {
      std::sort(frontline.begin(), frontline.end(), [](const std::tuple<std::tuple<unsigned int, unsigned int>, double> &a,
                   const std::tuple<std::tuple<unsigned int, unsigned int>, double> &b) {
                    return std::get<1>(a) < std::get<1>(b);
              });

      unsigned int prioX = std::get<0>(std::get<0>(frontline[frontline.size() - 1]));
      unsigned int prioY = std::get<1>(std::get<0>(frontline[frontline.size() - 1]));
      std::tuple<unsigned int, unsigned int> psi = image_map.nearest_patch(prioX, prioY);
      image_map.fill_from_patch(prioX, prioY, std::get<0>(psi), std::get<1>(psi));

      image_map.update_frontline(frontline);
    }

    std::string image_filename = std::string(OUTPUT_DIR) + "/region_filling_output.png";

    cv::Mat confidence_image(rows(), cols(), CV_8UC1);
    cv::Mat output_image(rows(), cols(), CV_8UC1);
    #pragma omp parallel for collapse(2) schedule(static)
    for (int i = 0; i < rows(); i++) {
          for (int j = 0; j < cols(); j++) {
              confidence_image.at<uchar>(i, j) = image_map.P[i][j].confidence*255;
              output_image.at<uchar>(i, j) = image_map.P[i][j].color_value;
          }
    }
    output_image.at<uchar>(rows() - 1, 0) = 0;
    confidence_image.at<uchar>(rows() - 1, 0) = 0;

    std::string confidence_path = std::string(OUTPUT_DIR) + "/confidence_image.png";
    if (!cv::imwrite(confidence_path, confidence_image)) {
        Logger::error("Failed to save the image to " + confidence_path);
    }

    if (cv::imwrite(image_filename, output_image)) {
        Logger::info("Image saved successfully as " + image_filename);
    } else {
        Logger::error("Failed to save the image to " + image_filename);
    }
  }
  else if (nb_channels == 3){
    Logger::debug("Processing a color image");

    cv::Mat bgrchannel[3];
    cv::split(data, bgrchannel);
    image img_blue(bgrchannel[0]);
    image img_green(bgrchannel[1]);
    image img_red(bgrchannel[2]);
    std::string link_output = std::string(OUTPUT_DIR) + "/region_filling_output.png";

    // The 3 channels are independent from each other — parallelizable in
    // theory, but each currently reads/writes the same "link_output" file
    // sequentially: needs 3 distinct temp files before parallelizing safely.
    img_blue.region_filling_recovery(coords);
    cv::Mat output_blue = cv::imread(link_output, cv::IMREAD_GRAYSCALE);

    img_green.region_filling_recovery(coords);
    cv::Mat output_green = cv::imread(link_output, cv::IMREAD_GRAYSCALE);

    img_red.region_filling_recovery(coords);
    cv::Mat output_red = cv::imread(link_output, cv::IMREAD_GRAYSCALE);

    std::vector<cv::Mat> array_to_merge = {output_blue, output_green, output_red};
    cv::Mat color;
    cv::merge(array_to_merge, color);

    std::string color_path = std::string(OUTPUT_DIR) + "/output_color.png";
    if (!cv::imwrite(color_path, color)) {
        Logger::error("Failed to save the image to " + color_path);
    }
  }
}

// DISPLAYING

std::ostream& operator <<(std::ostream &os, const image &I) {
  os << I.data;
  return os;
}

void image::imshow(){
    cv::imshow("The image:", data);
}

// SAVING

bool image::imwrite(const std::string &link) const {
  return cv::imwrite(link, data);
}

// ROTATIONS

// Classic rigid rotation: native backward mapping via warpAffine, no holes.
void image::rotation(const float& angle) {
    data.convertTo(data, CV_8U);
    cv::Point2f center(data.cols / 2.0f, data.rows / 2.0f);
    cv::Mat rotationMatrix = cv::getRotationMatrix2D(center, angle, 1.0);

    cv::Mat outputImage;
    cv::warpAffine(data, outputImage, rotationMatrix, data.size(),
                    cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(255));

    data = outputImage;
}

// Elastic rotation: per-pixel rotation strength depending on distance to center.
//
// FIX: the original implementation used forward mapping (for each source
// pixel, compute where it lands) — this is not safely parallelizable, since
// several source pixels can map to the same destination pixel and race on
// the write (undefined result once run with OpenMP).
//
// A rotation preserves distance to its center, so the distance computed at
// a destination pixel equals the distance that would be computed at its
// corresponding source pixel. This lets us invert the transform directly:
// for each destination pixel, compute the strength/angle from ITS OWN
// distance to the center, apply the inverse rotation to find the source
// coordinates, and sample there. This is backward mapping — no holes, no
// race condition, safely parallelizable.
void image::elasity_rotation(const float& angle){
    data.convertTo(data, CV_8U);
    cv::Mat outputImage = data.clone();

    float radius = data.cols / 4.0f;
    float centerX = 208.0f;
    float centerY = 453.0f;
    int rows = data.rows;
    int cols = data.cols;

    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int Y = 0; Y < rows; Y++) {
        for (int X = 0; X < cols; X++) {
            float distance = std::sqrt(std::pow(X - centerX, 2) + std::pow(Y - centerY, 2));

            if (distance < radius) {
                float strength = std::min(radius / (2 * distance), 1.0f);

                // Inverse rotation: negative angle to go from destination back to source
                cv::Mat rotationMatrix = cv::getRotationMatrix2D(cv::Point(centerX, centerY), -angle * strength, 1.0);
                rotationMatrix.convertTo(rotationMatrix, CV_32F);

                cv::Mat destVector = (cv::Mat_<float>(3, 1) << (float)X, (float)Y, 1.0f);
                cv::Mat sourceVector = rotationMatrix * destVector;

                float srcX = sourceVector.at<float>(0, 0);
                float srcY = sourceVector.at<float>(1, 0);

                if (srcX >= 0 && srcX < cols - 1 && srcY >= 0 && srcY < rows - 1) {
                    outputImage.at<uchar>(Y, X) = bilinear_interpolation(srcX, srcY);
                }
                // Out-of-bounds source: destination pixel keeps its cloned original value.
            }
            // Outside the elastic radius: destination pixel keeps its cloned original value.
        }
    }

    data = outputImage;
    // No hole-filling pass needed anymore: backward mapping guarantees every
    // destination pixel is assigned exactly once.
}

// General bilinear sampling at arbitrary floating-point source coordinates.
// Replaces the previous version, which assumed integer (x, y) close to
// (new_x, new_y) — not valid for an arbitrary inverse-mapped source point.
float image::bilinear_interpolation(const float& src_x, const float& src_y) {
    int x0 = static_cast<int>(std::floor(src_x));
    int y0 = static_cast<int>(std::floor(src_y));
    int x1 = std::min(x0 + 1, data.cols - 1);
    int y1 = std::min(y0 + 1, data.rows - 1);

    float dx = src_x - x0;
    float dy = src_y - y0;

    int f00 = static_cast<int>(data.at<uchar>(y0, x0));
    int f10 = static_cast<int>(data.at<uchar>(y0, x1));
    int f01 = static_cast<int>(data.at<uchar>(y1, x0));
    int f11 = static_cast<int>(data.at<uchar>(y1, x1));

    return f00 * (1 - dx) * (1 - dy) +
           f10 * dx * (1 - dy) +
           f01 * (1 - dx) * dy +
           f11 * dx * dy;
}