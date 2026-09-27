#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <opencv2/opencv.hpp>
#include <iostream>

// The following implementations have all worked during my tests.
// Make sure they work for you aswell.

/**
 * @brief Wrapper around OpenCV's cv::Mat providing image manipulation,
 * rotation, convolution, and region-filling (inpainting) operations.
 */
class image {
friend class map; // class "map" is allowed to access all data from this class
public :
    cv::Mat data;

    /**
     * @brief Number of channels the image has.
     *
     * By default, set to 1 (grayscale, grey intensity values). 3 is used for
     * color images. Beware: OpenCV uses the "BGR" convention, not "RGB", so
     * pixels must be read in the correct order.
     *
     * Regardless of channel count, all pixels are stored in the same matrix
     * at the same level — the first free coefficients encode the color of
     * the FIRST pixel (in the 3-channel case). Following the BGR convention,
     * the first coefficient is blue, the second green, the third red.
     *
     * By default, the "data" matrix should have an integer type like CV_8U
     * before reading. Otherwise, convert it first with the existing
     * "convertTo" method (whose parameter is the target type).
     */
    unsigned int nb_channels;

public :
    // CONSTRUCTORS  

    /** @brief Default constructor. Initializes a black 250x250 grayscale image. */
    image();

    /**
     * @brief Constructs an image by loading it from a file path.
     * @param link Path to the image file (C-string).
     */
    image(const char *);

    /**
     * @brief Constructs an image by loading it from a file path.
     * @param link Path to the image file (std::string).
     */
    image(const std::string);

    /**
     * @brief Constructs an image by loading it from a file path, with explicit channel count.
     * @param link Path to the image file.
     * @param nb Number of channels to initialize (see nb_channels).
     */
    image(const char *, const unsigned int &);

    /**
     * @brief Constructs an image from an existing OpenCV matrix.
     * @param M Source matrix.
     */
    image(const cv::Mat &);

    /**
     * @brief Copy constructor.
     * @param I Image to copy.
     */
    image(const image &);

    // OPERATOR OVERLOADINGS

    /**
     * @brief Compares two images by the sum of their pixel values.
     * @param M Image to compare against.
     * @return True if this image's pixel sum is less than or equal to M's.
     */
    bool operator <=(const image &) const;

    /**
     * @brief Assigns an OpenCV matrix to this image, converting type if needed.
     * @param M Matrix to assign.
     * @return Reference to *this.
     */
    image operator =(const cv::Mat &);


    // REIMPLEMENTATION OF PREXISTING METHODS OF CLASS "Mat" (from opencv)

    /** @brief Returns the number of rows the image matrix has. */
    int rows() const;

    /**
     * @brief Returns an image made of the selected row.
     * @param i Index of the chosen row.
     */
    image row(const unsigned int &) const;

    /** @brief Returns the number of columns the image matrix has. */
    int cols() const;

    /**
     * @brief Returns an image made of the selected column.
     * @param i Index of the chosen column.
     */
    image col(const unsigned int &) const;


    // MAIN OPERATIONS

    /** @brief Flips the image symmetrically with respect to the X axis. */
    void symetry_Y();

    /** @brief Flips the image symmetrically with respect to the Y axis. */
    void symetry_X();

    /** @brief Transposes the object's matrix. */
    void t();

    /** @brief Transposes the object's matrix with respect to the 2nd diagonal. */
    void t2();

    /** @brief Normalizes the matrix / returns the equivalent matrix with float values in [0, 1]. */
    void normalize();

    /** @brief Normalizes the image to double-precision channel values in [0, 1]. */
    void normalize2d();

    /** @brief Stretches pixel values to the full [0, 255] range and converts back to 8-bit. */
    void stretch2d();

    /**
     * @brief Recovers an approximation of the matrix from its normalized form.
     * @note This operation is approximate due to encoding limits for small floats.
     */
    void stretch();

    /**
     * @brief Applies a 2D convolution to the image using the given kernel.
     * @param kernel Convolution kernel (normalized internally).
     * @return The convolved image.
     */
    image convolve(cv::Mat);

    // METHODS TO ALTER IMAGES

    /**
     * @brief Fills a rectangular region of the image with a solid color.
     * @param color Color value to apply.
     * @param r1 Row of the first corner.
     * @param c1 Column of the first corner.
     * @param r2 Row of the opposite corner.
     * @param c2 Column of the opposite corner.
     */
    void set_color_rectangle(const unsigned int &, const unsigned int &, const unsigned int &, const unsigned int &, const unsigned int &);

    // MAIN ALGORITHM IMPLEMENTATIONS

    /**
     * @brief Replaces and fills up a rectangular region within the image using the
     * region-filling (inpainting) algorithm.
     *
     * In our field of study, we are only asked a priori to handle such a rectangular
     * domain, since supporting arbitrary shapes would require designing a more complex
     * selection interface. For instance, the given fingerprint images only have such
     * domains to fill.
     *
     * The output image is saved in the "output images" directory of the project.
     *
     * @param coords Coordinates of the rectangular domain: (r1, c1) is the top-left
     * pixel, (r2, c2) is the bottom-right pixel.
     */
    void region_filling_recovery(const std::vector<unsigned int>&) const;

    // DISPLAYING

    /** @brief Stream insertion operator, delegating to cv::Mat's own ostream overload. */
    friend std::ostream& operator <<(std::ostream&, const image&);

    /** @brief Displays the image in an OpenCV window. */
    void imshow();

    // SAVING

    /**
     * @brief Saves the image to the given path.
     * @param link Destination path, including the folder name and file extension (e.g. .png).
     * @return True if the image was saved successfully.
     */
    bool imwrite(const std::string &) const;

    /**
     * @brief Rotates the image by a given angle around its center, using backward
     * mapping (no holes in the output).
     * @param angle Rotation angle in degrees.
     */
    void rotation(const float& angle);

    /**
     * @brief Samples the image at arbitrary floating-point coordinates using
     * bilinear interpolation.
     * @param src_x Source x-coordinate (can be non-integer).
     * @param src_y Source y-coordinate (can be non-integer).
     * @return Interpolated pixel value.
     */
    float bilinear_interpolation(const float& src_x, const float& src_y);

    /**
     * @brief Applies a rotation whose strength varies per pixel depending on the
     * distance to a fixed center, using backward mapping.
     * @param angle Maximum rotation angle in degrees, scaled by local strength.
     */
    void elasity_rotation(const float& angle);

    /**
     * @brief Fills small unfilled gaps in the image by averaging neighboring pixels.
     * @param x Column of the pixel to refine.
     * @param y Row of the pixel to refine.
     * @return Averaged value, or -1 if the pixel does not qualify for refinement.
     */
    float refineImage(const int& x, const int& y);

};

#endif