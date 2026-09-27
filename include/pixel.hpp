#ifndef PIXEL_HPP
#define PIXEL_HPP

#include <iostream>
#include <opencv2/opencv.hpp>

/**
 * @brief Represents a single pixel's state for the region-filling (inpainting) algorithm.
 *
 * Tracks both the grayscale intensity and a confidence value indicating how
 * trustworthy/known that pixel currently is.
 */
class pixel {
friend class map; // class "map" is allowed to access all data from this class

public :
    /** @brief Grayscale intensity value, in [0, 255]. */
    uchar color_value;

    /**
     * @brief Confidence of the pixel's value.
     *
     * 0 means unknown, 1 means initially known (trustworthy), and any value
     * strictly between 0 and 1 means the pixel has been filled by the algorithm.
     */
    double confidence;

public :
    // CONSTRUCTORS

    /** @brief Default constructor. Initializes color_value and confidence to their default values. */
    pixel();

    /**
     * @brief Constructs a pixel with explicit color value and confidence.
     * @param color_value Grayscale intensity, in [0, 255]. Default: 0.
     * @param confidence Confidence value. Default: 1.
     */
    pixel(const uchar &, const double &);

    /**
     * @brief Copy constructor.
     * @param p Pixel to copy.
     */
    pixel(const pixel &);

    // DISPLAYING (mainly for test and check purposes)

    /** @brief Stream insertion operator for debugging/inspection purposes. */
    friend std::ostream & operator << (std::ostream &, const pixel &);
};

#endif