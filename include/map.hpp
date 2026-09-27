#ifndef MAP_HPP
#define MAP_HPP

#include <iostream>
#include <vector>
#include <tuple>

#include "pixel.hpp"
#include "image.hpp"

// This class only serves for the "Region Filling Algorithm" and maps an image with a matrix of pixels having attributes
// used in the algorithm to compute patch priorities, confidence and such.
// Most importantly, it allows the program to understand where the filling domain is wrt the image (due to the "filled" attribute of each pixel)

// dimensions of each patch
#define DEF_HEIGHT 15 // they have to be equal and an odd value !
#define DEF_WIDTH 15 // (so that there is a mid pixel inside each patch)

/**
 * @brief Maps an image to a grid of pixel structures carrying the attributes
 * (confidence, color value) needed by the region-filling (inpainting) algorithm.
 *
 * Most importantly, it lets the algorithm know where the filling domain lies
 * relative to the image, via each pixel's confidence/filled state.
 */
class map {
friend class image; // class "image" is allowed to access all data from this class
public :
    pixel ** P;
    unsigned int rows, cols; // dimensions of the matrix (just to make things a tiny more comfortable)

public :

    //CONSTRUCTORS

    // no default constructor is needed since we always generate the map from an image matrix

    /**
     * @brief Constructs a map from an image and a rectangular domain to fill.
     * @param I Source image.
     * @param r1 Row of the first corner of the rectangular domain.
     * @param c1 Column of the first corner of the rectangular domain.
     * @param r2 Row of the opposite corner of the rectangular domain.
     * @param c2 Column of the opposite corner of the rectangular domain.
     */
    map(const image &, const unsigned int &, const unsigned int &, const unsigned int &, const unsigned int &);

    /**
     * @brief Constructs a map from an image and a polygonal domain to fill.
     *
     * By convention, the polygon's first vertex starts a horizontal edge,
     * and therefore its last vertex ends a vertical edge (for easier implementation).
     *
     * @param I Source image.
     * @param coords Flattened list of polygon vertex coordinates (r1, c1, r2, c2, ...).
     */
    map(const image &, const std::vector<unsigned int> &);

    /**
     * @brief Copy constructor.
     * @param m Map to copy.
     */
    map(const map &);

    /** @brief Destructor. Frees the dynamically allocated pixel grid. */
    ~map();

    // METHODS USED PRIMARLY FOR THE REGION_FILLING_ALGORITHM 

    /**
     * @brief Computes the normal vector at the given coordinates, assuming no
     * issues related to image borders.
     * @param r Row coordinate.
     * @param c Column coordinate.
     * @return Normalized normal vector as a tuple (i, j).
     */
    std::tuple<double,double> compute_normal(const unsigned int &, const unsigned int &) const;

    /**
     * @brief Checks whether a valid Sobel gradient can be computed at the given
     * coordinates (i.e. within bounds and fully surrounded by filled pixels).
     * @param r Row coordinate.
     * @param c Column coordinate.
     * @return True if the gradient is computable.
     */
    bool viable_gradient(const unsigned int &, const unsigned int &) const;

    /**
     * @brief Finds the strongest Sobel gradient vector in the neighborhood
     * around the given coordinates.
     * @param r Row coordinate.
     * @param c Column coordinate.
     * @return Tuple (g_x, g_y) representing the strongest gradient found.
     */
    std::tuple<int, int> gradient(const unsigned int &, const unsigned int &) const;

    /**
     * @brief Computes the confidence value of the patch centered at the given coordinates.
     * @param r Row coordinate.
     * @param c Column coordinate.
     * @return Average confidence over the patch.
     */
    double compute_confidence(const unsigned int &, const unsigned int &) const;

    /**
     * @brief Computes the filling priority of the patch centered at the given coordinates.
     * @param r Row coordinate.
     * @param c Column coordinate.
     * @return Priority value (confidence times data term).
     */
    double compute_priority_patch(const unsigned int &, const unsigned int &) const;

    /**
     * @brief Computes the least-squares distance between two patches, considering
     * only filled pixels in both.
     * @param r1 Row of the first patch's center.
     * @param c1 Column of the first patch's center.
     * @param r2 Row of the second patch's center.
     * @param c2 Column of the second patch's center.
     * @return Sum of squared differences over filled pixels.
     */
    double distance_from_patches(const unsigned int &, const unsigned int &, const unsigned int &, const unsigned int &) const;

    /**
     * @brief Fills the unfilled pixels of a patch using another (source) patch.
     * Sets the confidence of newly filled pixels to that of the destination patch center.
     * @param r1 Row of the destination patch's center.
     * @param c1 Column of the destination patch's center.
     * @param r2 Row of the source patch's center.
     * @param c2 Column of the source patch's center.
     */
    void fill_from_patch(const unsigned int &, const unsigned int &, const unsigned int &, const unsigned int &);

    /**
     * @brief Checks whether the entire patch at the given coordinates lies inside
     * the known region (all pixels have confidence 1).
     * @param r Row coordinate.
     * @param c Column coordinate.
     * @return True if the whole patch is filled.
     */
    bool patch_inside(const unsigned int &, const unsigned int &) const;

    /**
     * @brief Finds the coordinates of the patch closest (least-squares distance)
     * to the patch at the given pixel coordinates.
     * @param r Row coordinate.
     * @param c Column coordinate.
     * @return Coordinates of the nearest matching patch.
     */
    std::tuple<unsigned int, unsigned int> nearest_patch(const unsigned int &, const unsigned int &) const;

    /**
     * @brief Builds the initial front line (border) of the domain to fill.
     *
     * Only works with a rectangular domain specified via both coordinates
     * given to the region-filling algorithm.
     *
     * @param coords Flattened list of domain vertex coordinates.
     * @return Vector of (pixel coordinates, priority) pairs along the border.
     */
    std::vector<std::tuple<std::tuple<unsigned int, unsigned int>, double>> Fill_Front(const std::vector<unsigned int> &) const;

    /**
     * @brief Updates the front line after a pixel has been filled: removes
     * pixels no longer on the border and adds newly exposed ones.
     * @param frontline Front line to update in place.
     */
    void update_frontline(std::vector<std::tuple<std::tuple<unsigned int, unsigned int>, double>> &);

    // OPERATOR OVERLOADS

    /**
     * @brief Assignment operator. Deep-copies the pixel grid.
     * @note Kept for safety since the class holds dynamic data, though it
     * shouldn't actually be used in practice.
     * @param pat Map to assign from.
     * @return Reference to *this.
     */
    map operator =(const map &);
};

#endif