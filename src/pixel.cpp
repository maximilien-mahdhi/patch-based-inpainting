#include <iostream>

#include "pixel.hpp"

// Default constructor: black pixel, fully trusted (assumed to be an
// original, unmodified pixel). It is easier to explicitly mark pixels
// to be filled than the other way around, hence this default.
pixel::pixel()
: color_value(0),
  confidence(1)
{}

pixel::pixel(const uchar &color, const double &confid)
: color_value(color), confidence(confid) {}

pixel::pixel(const pixel &p)
: color_value(p.color_value), confidence(p.confidence) {}

// Stream insertion operator, mainly for debugging/inspection purposes.
std::ostream& operator << (std::ostream& os, const pixel &p) {
    os << "Data of the pixel:" << std::endl;
    os << "Color value: " << (int) p.color_value << std::endl;
    os << "Status: ";
    if (p.confidence != 0) {
        os << "filled";
    } else {
        os << "empty";
    }
    os << std::endl;
    os << "Confidence: " << p.confidence << std::endl;

    return os;
}