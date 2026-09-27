#include "interactive_selection.hpp"
#include "logger.hpp"
#include "config.hpp"
#include <iostream>

namespace {

// === Internal state for the mouse callback (kept local to this file) ===
std::vector<unsigned int> mouse_coords;
bool next_horizontal = true;   // true = next point shares the same row (r), false = same column (c)
cv::Mat base_image;
cv::Mat preview_image;

/**
 * @brief True if the polygon could be closed validly right now: map.cpp always
 *        draws the closing edge (last point -> first point) as vertical, anchored
 *        at the first point's column, so the last point's column must match it.
 */
bool closure_is_valid() {
    if (mouse_coords.size() < 6) return false;  // need at least 3 points
    unsigned int first_c = mouse_coords[1];
    unsigned int last_c  = mouse_coords[mouse_coords.size() - 1];
    return first_c == last_c;
}

/**
 * @brief Redraws the selection window: validated segments, clicked points,
 *        a live guide line snapped to horizontal/vertical toward the mouse,
 *        and a preview of the closing edge (green if valid, orange otherwise).
 */
void draw_preview(int mouse_x, int mouse_y) {
    preview_image = base_image.clone();

    // Redraw all already-validated segments
    for (size_t k = 0; k + 3 < mouse_coords.size(); k += 2) {
        cv::Point p1(mouse_coords[k+1], mouse_coords[k]);     // (c, r) since OpenCV expects (x, y)
        cv::Point p2(mouse_coords[k+3], mouse_coords[k+2]);
        cv::line(preview_image, p1, p2, cv::Scalar(0, 255, 0), 1);
    }
    for (size_t k = 0; k < mouse_coords.size(); k += 2) {
        cv::Point p(mouse_coords[k+1], mouse_coords[k]);
        cv::circle(preview_image, p, 3, cv::Scalar(0, 0, 255), -1);
    }

    // Live guide line, snapped to the axis expected for the next point
    if (!mouse_coords.empty()) {
        unsigned int last_r = mouse_coords[mouse_coords.size() - 2];
        unsigned int last_c = mouse_coords[mouse_coords.size() - 1];
        cv::Point last(last_c, last_r);
        cv::Point guide;
        if (next_horizontal) {
            guide = cv::Point(mouse_x, last_r);   // same r, free c
        } else {
            guide = cv::Point(last_c, mouse_y);   // same c, free r
        }
        cv::line(preview_image, last, guide, cv::Scalar(255, 0, 0), 1);
    }

    // Preview of the closing edge (last point back to first point):
    // green if it would be a valid vertical closure, orange as a warning otherwise
    if (mouse_coords.size() >= 6) {
        cv::Point first(mouse_coords[1], mouse_coords[0]);
        cv::Point last(mouse_coords[mouse_coords.size()-1], mouse_coords[mouse_coords.size()-2]);
        cv::Scalar closure_color = closure_is_valid() ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 165, 255);
        cv::line(preview_image, last, first, closure_color, 1, cv::LINE_AA);
    }

    cv::imshow("Selection", preview_image);
}

/**
 * @brief Clamps a coordinate to valid image bounds, guarding against
 *        out-of-range clicks (e.g. display scaling mismatches).
 */
int clamp(int value, int max_exclusive) {
    if (value < 0) return 0;
    if (value >= max_exclusive) return max_exclusive - 1;
    return value;
}

/**
 * @brief Mouse callback for interactive polygon selection.
 *        Left click adds a point, snapped to alternate horizontal/vertical
 *        segments (same constraint as the -i command-line format).
 */
void on_mouse(int event, int x, int y, int flags, void* userdata) {
    x = clamp(x, base_image.cols);
    y = clamp(y, base_image.rows);

    if (event == cv::EVENT_MOUSEMOVE) {
        draw_preview(x, y);
    }
    else if (event == cv::EVENT_LBUTTONDOWN) {
        if (mouse_coords.empty()) {
            // First point: fully free
            mouse_coords.push_back(static_cast<unsigned int>(y));  // r
            mouse_coords.push_back(static_cast<unsigned int>(x));  // c
        } else {
            unsigned int last_r = mouse_coords[mouse_coords.size() - 2];
            unsigned int last_c = mouse_coords[mouse_coords.size() - 1];
            if (next_horizontal) {
                mouse_coords.push_back(last_r);                        // same r
                mouse_coords.push_back(static_cast<unsigned int>(x));  // new c
            } else {
                mouse_coords.push_back(static_cast<unsigned int>(y));  // new r
                mouse_coords.push_back(last_c);                        // same c
            }
        }
        next_horizontal = !next_horizontal;
        draw_preview(x, y);
    }
}

} // anonymous namespace

std::vector<unsigned int> select_polygon_interactive(const std::string& file_name) {
    // Reset state in case this is called more than once in the same run
    mouse_coords.clear();
    next_horizontal = true;

    base_image = cv::imread(file_name);
    if (base_image.empty()) {
        Logger::error("Unable to load the image");
        return {};
    }
    Logger::info("Image read successfully!");

    cv::namedWindow("Selection");
    cv::setMouseCallback("Selection", on_mouse);
    draw_preview(0, 0);

    std::cout << "Click the polygon vertices (segments alternate horizontal/vertical).\n";
    std::cout << "The last point must line up in the same column as the first (closing edge is vertical).\n";
    std::cout << "Enter = confirm, Escape = cancel, Backspace = undo last point.\n";

    while (true) {
        int key = cv::waitKey(0);
        if (key == 13) {
            if (closure_is_valid()) {
                break;
            } else {
                Logger::warning("The polygon is not closing correctly: the last point must have the same column as the first one.");
            }
        }
        if (key == 27) { cv::destroyWindow("Selection"); return {}; }  // Escape
        if (key == 127 || key == 8) {                                  // Backspace
            if (mouse_coords.size() >= 2) {
                mouse_coords.pop_back();
                mouse_coords.pop_back();
                next_horizontal = !next_horizontal;
                draw_preview(0, 0);
            }
        }
    }

    std::string preview_path = std::string(OUTPUT_DIR) + "/selection_preview.png";
    if (!cv::imwrite(preview_path, preview_image)) {
        Logger::error("Failed to save the image to " + preview_path);
    } else {
        Logger::info("Selection preview saved to " + preview_path);
    }

    cv::destroyWindow("Selection");

    return mouse_coords;
}