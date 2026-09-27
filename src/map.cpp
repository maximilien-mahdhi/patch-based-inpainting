#include <iostream>
#include <algorithm>
#include <omp.h>

#include "pixel.hpp"
#include "map.hpp"


// we assume that two segments can not be aligned, otherwise -> problem

map::map(const image& I, const std::vector<unsigned int>& coords) 
: rows(I.rows()), cols(I.cols()) {

    //Initialization
    I.data.convertTo(I.data, CV_8U); // making sure the image has type CV_8U (gray values in [0, 255] : very important !)
    P = new pixel*[rows];
    for (int i = 0; i < rows; i++) {
        P[i] = new pixel[cols];
    }

    //Construction of the border of the polygone to be filled
    //We need to deal with the last vertice of the polygone first as we will get rid of the first point of the polygone 74 22 74 79 116 79 116 22
    unsigned int r = coords[0]; 
    unsigned int c = coords[1];
    unsigned int next_r = coords[coords.size()-2];
    unsigned int next_c = coords[coords.size()-1];
    for (unsigned int i = std::min(r,next_r); i <= std::max(r,next_r); i++) //As the last vertice is by convention vertical
            {
                P[i][c].color_value = 255;  // given some arbitrary and insane value to make strong differentiation with other pixel values
                P[i][c].confidence = 0;// for now, confidence is set to 0 while original pixels (outside the domain) are 100% trustworthy
            }

    for (unsigned int k = 2; k < coords.size(); k+= 2) //Là et dans fill_front et bah il manque le dernier segment
    {
        next_r = coords[k];
        next_c = coords[k+1];
        if (r == next_r)
        {
            for (unsigned int j = std::min(c,next_c); j <= std::max(c,next_c); j++)
            {
                
                P[r][j].color_value = 255;  // given some arbitrary and insane value to make strong differentiation with other pixel values
                P[r][j].confidence = 0;// for now, confidence is set to 0 while original pixels (outside the domain) are 100% trustworthy
            }
        }
        else if (c == next_c){
            for (unsigned int i = std::min(r,next_r); i <= std::max(r,next_r); i++)
            {
                P[i][c].color_value = 255;  // given some arbitrary and insane value to make strong differentiation with other pixel values
                P[i][c].confidence = 0;// for now, confidence is set to 0 while original pixels (outside the domain) are 100% trustworthy
            }
        }
        else{
            std::cout << "Points of the polygone should be on the same row or on the same column" << std::endl;
        }
        r = next_r;
        c = next_c;
    }
    
    //Filling of the image thanks to the border
    for (unsigned int i = 0; i < rows; i++)
    {
        unsigned int counter = 0; //This variable will counts the amount of time we pass across the border to know if we are inside or not
        unsigned int ghost_counter = 0;
        bool was_in_border = false;
        unsigned int parity = 0;
        for (unsigned int j = 0; j < cols; j++)
        {
            parity = counter & 1;
            if (P[i][j].confidence == 0 && !was_in_border) //Meaning that it is a point of the border
            {
                
                bool cross = false;
                ghost_counter = counter;
                for (unsigned int j = c; j < cols -1; j++)
                {
                    if (P[i][j].confidence == 0 && P[i][j-1].confidence != 0 && P[i][j+1].confidence != 0)
                    {
                        ghost_counter++;
                    } 
                }
                unsigned int test_even = ghost_counter & 1;

                if (test_even == 0)
                {
                    cross = false;
                }
                else{
                    cross = true;
                }

                if (P[i][j+1].confidence != 0|| cross){
                    counter++;
                }
                was_in_border = true;
            }
            else if (parity == 1 && P[i][j].confidence == 1){ // If we are inside of the polygone (border not included)
                P[i][j].color_value = 255;  // given some arbitrary and insane value to make strong differentiation with other pixel values
                P[i][j].confidence = 0;// for now, confidence is set to 0 while original pixels (outside the domain) are 100% trustworthy
                was_in_border = false;
            }
            else if (parity == 0 && P[i][j].confidence == 1){
                P[i][j].color_value = I.data.at<uchar>(i, j);
                was_in_border = false;
            }
        }
    }
}

map::map(const map &m) 
: rows(m.rows), cols(m.cols)
{
    P = new pixel * [rows];
    for (int i = 0; i < rows; i++) {
        P[i] = new pixel[cols];
        for (int j = 0; j < cols; j++) {
            P[i][j] = m.P[i][j];
        }
    }
}

map::~map() {
    for (int i = 0; i < rows; i++) {
        delete [] P[i];
    }
    delete [] P;
}

bool is_adjacent(const std::tuple<unsigned int, unsigned int> &coord, const unsigned int &i, const unsigned int &j){ //true iff adjacent (not diagonal)
    int diff_i = std::get<0>(coord) - i;
    diff_i = std::abs(diff_i);
    int diff_j = std::get<1>(coord) - j;
    diff_j = std::abs(diff_j);
    if  ((diff_i <= 1) && (diff_j <= 1) && (diff_i != diff_j)){
        return true;
    }
    else{
        return false;
    }
}
bool in_patch(const unsigned int &i, const unsigned int &j, const unsigned int &r, const unsigned int &c){ //true iff inside the patch but not in the middle
    int diff_i = r - i;
    diff_i = std::abs(diff_i);
    int diff_j = c - j;
    diff_j = std::abs(diff_j);
    if  ((diff_i <= 1) && (diff_j <= 1)){
        return true;
    }
    else{
        return false;
    }
}

std::tuple<double,double> normalize(double i, double j){ //normalize the vector (i,j) and returns it as a tuple
    double coef = pow(i*i +j*j,0.5);
    return std::tuple<double,double>(i/coef,j/coef);
}

// NOT parallelized: builds dynamic std::vector<std::vector<...>> structures with
// push_back/insert/erase depending on prior iterations — inherently sequential,
// and the 3x3 patch scope makes its cost negligible compared to nearest_patch.
std::tuple<double,double> map::compute_normal(const unsigned int &r, const unsigned int &c) const {

    std::vector<std::vector<std::tuple<unsigned int,unsigned int>>> borders; //We will stock the multiple connected borders in this variable
    //Two known points are considered from different borders if those borders are not connected

    std::vector<unsigned int> adjacent_to; //For a given pixel, this vector is filled with the index of the borders which are adjacent to it

    //We are only interested in the 3x3 patch centered at the points
    //We want to find the known/filled points in this patch to determine the border and thus the normal vector
    for (int i = r-1; i <= r+1; i++)
    {
        for (int j = c-1; j <= c+1; j++)        
        {
            if (P[i][j].confidence != 0) //If P[i][j] is known
            {
                if (borders.empty()) //If there is no border, we create the first one
                {
                    std::vector<std::tuple<unsigned int, unsigned int>> tmp = {std::tuple<unsigned int, unsigned int>(i,j)};
                    borders.push_back(tmp);
                }
                else //Otherwise we want to know to which borders this known pixel is adjacent
                {
                    adjacent_to.clear();

                    for (int k = 0; k <  borders.size(); k++)
                    {
                        for (int p = 0; p < (borders[k]).size(); p++) //not useful to iterate every points if one is adjacent -> while
                        {
                            if (is_adjacent(borders[k][p],i,j))
                            {
                                adjacent_to.push_back(k);
                            }
                        }
                    }    
                    if (adjacent_to.size() == 2) // then we have a single border that is the concatenation of those borders and the new point
                    {
                        borders[adjacent_to[0]].push_back(std::tuple<unsigned int, unsigned int>(i,j));
                        borders[adjacent_to[0]].insert(borders[adjacent_to[0]].end(),borders[adjacent_to[1]].begin(),borders[adjacent_to[1]].end());
                        borders.erase(borders.begin() + adjacent_to[1]);
                    }
                    else if (adjacent_to.size() == 1) // then we add the point to this border
                    {
                        borders[adjacent_to[0]].push_back(std::tuple<unsigned int, unsigned int>(i,j));
                    }
                    else // we add the point in a new border
                    {
                    std::vector<std::tuple<unsigned int, unsigned int>> tmp = {std::tuple<unsigned int, unsigned int>(i,j)};
                    borders.push_back(tmp);
                    } 
                }
            }
        }
    }
    
    //At this point borders contains a vector of tuple for every non connected borders in the 3x3 patch
    //We will compute the sum of the confidence of those borders in order to choose the one we have the most confidence in
    int i,j;
    std::vector<double> conf_vector;
    double total_conf = 0;
    double conf;
    for (int k = 0; k < borders.size(); k++)
    {
        conf = 0;
        for (int p = 0; p < borders[k].size(); p++)
        {
            i = std::get<0>(borders[k][p]);
            j = std::get<1>(borders[k][p]);
            conf += P[i][j].confidence;
        }
        total_conf += conf;
        conf_vector.push_back(conf);    
    }
    //we now have the confidence of each borders, which will serve to compute the average
    std::vector<std::vector<std::tuple<unsigned int, unsigned int>>> unknown_borders; 
    std::vector<std::tuple<int,int>> tangent_vectors; //

    for (int k = 0; k < borders.size(); k++)
    {
        std::vector<std::tuple<unsigned int, unsigned int>> tmp2;
        for (int p = 0; p < borders[k].size(); p++)
        {
            i = std::get<0>(borders[k][p]);
            j = std::get<1>(borders[k][p]);
            if  (in_patch(i-1,j,r,c) && P[i-1][j].confidence == 0 && (i-1 != r || j != c))
            {                                                 
                tmp2.push_back(std::tuple<unsigned int, unsigned int>(i-1,j)); 
            }
            if  (in_patch(i+1,j,r,c) && P[i+1][j].confidence == 0 && (i+1 != r || j != c))
            {                                                  
                tmp2.push_back(std::tuple<unsigned int, unsigned int>(i+1,j)); 
            }
            if  (in_patch(i,j-1,r,c) && P[i][j-1].confidence == 0 && (i != r || j-1 != c))
            {                                                
                tmp2.push_back(std::tuple<unsigned int, unsigned int>(i,j-1)); 
            }
            if  (in_patch(i,j+1,r,c) && P[i][j+1].confidence == 0 && (i != r || j+1 != c))
            {                                                   
                tmp2.push_back(std::tuple<unsigned int, unsigned int>(i,j+1)); 
            }
        }
        unknown_borders.push_back(tmp2);
    }
    for (int k = 0; k < borders.size(); k++){
        //As we do not need the sense of the normal vector, only the direction, I decided to orientate them all
        //in a semi-plane so they do not cancel out in the case of symmetry (more explanation in person)
        if (unknown_borders[k].size() == 2) //usual case
        {
            int i1 = std::get<0>(unknown_borders[k][0]);
            int j1 = std::get<1>(unknown_borders[k][0]);
            int i2 = std::get<0>(unknown_borders[k][1]);
            int j2 = std::get<1>(unknown_borders[k][1]);

            if (j2-j1 < 0 || (j2-j1 == 0 && i2-i1 <=0) )
            {
                tangent_vectors.push_back(std::tuple<int,int>(i1-i2,j1-j2));
            }
            else{
                tangent_vectors.push_back(std::tuple<int,int>(i2-i1,j2-j1));
            }
        }
        else if (unknown_borders.size() == 1) //Case where there is an unique border surrounding the centered point with the exeption of a single point
        {                                     //Arbitrary decison might be good might be bad meh
            i = std::get<0>(unknown_borders[k][0]);
            j = std::get<1>(unknown_borders[k][0]); 
            if ((j - c < 0 || (j-c == 0 && i-r <= 0) ))
            {
                tangent_vectors.push_back(std::tuple<int,int>(r-i,c-j));
            }
            else{
                tangent_vectors.push_back(std::tuple<int,int>(i-r,j-c));
            }
            
        }
        else{ //Case where a single border totally surrounds the centered point 
            return std::tuple<double,double>(10e-6,10e-6); //Thus the normal vector can not be computed, and I chose to put an arbitrary low value
        }                                             //so that point would be computed last, but if there were multiples points in this case
                                                      //the value of confidence would still be relevant
    }
    //Computation of the normal vector based on the tangent vector and their trust
    //Remark : all the tangent vectors should be oriented toward the same semi-plane and therefore should not cancel each other
    double normal_i = 0;
    double normal_j = 0;
    for (int k = 0; k < tangent_vectors.size(); k++)
    {
        normal_i += -std::get<1>(tangent_vectors[k])*conf_vector[k]/total_conf;
        normal_j += std::get<0>(tangent_vectors[k])*conf_vector[k]/total_conf;
    }
    return normalize(normal_i,normal_j);
}

// FIX: the original loop always read P[r-1][c+1], P[r][c+1], etc. (fixed r, c),
// never using the loop variables i, j — so every iteration recomputed the exact
// same Sobel gradient at the center point, and the "search for the max gradient
// in the neighborhood" never actually happened. Changed to use i, j so the
// search is real. Confirm this matches the intended behavior.
std::tuple<int, int> map::gradient(const unsigned int &r, const unsigned int &c) const {
    int gap = (DEF_HEIGHT - 1) / 2;
    std::tuple<int, int> grad = std::make_tuple(0, 0); // lowest amplitude vector possible so it will be beaten
    for (int i = r - gap; i < r + gap + 1; i++) {
        for (int j = c - gap; j < c + gap + 1; j++) {
            if ((viable_gradient(i, j))) {
                // sobel computation, centered on (i, j) instead of the fixed (r, c)
                int g_x = P[i-1][j+1].color_value - P[i-1][j-1].color_value + 2 * (P[i][j+1].color_value - P[i][j-1].color_value) + P[i+1][j+1].color_value - P[i+1][j-1].color_value;
                int g_y = P[i+1][j-1].color_value - P[i-1][j-1].color_value + 2 * (P[i+1][j].color_value - P[i-1][j].color_value) + P[i+1][j+1].color_value - P[i-1][j+1].color_value;

                double buffer_squared_norm = pow(g_x, 2) + pow(g_y, 2); // storing squared norm of the computed gradient vector
                if (buffer_squared_norm > pow(std::get<0> (grad), 2) + pow(std::get<1> (grad), 2)) {
                    std::get<0> (grad) = g_x;
                    std::get<1> (grad) = g_y;
                }
            }
        }
    }
    return grad;
}

double map::compute_confidence(const unsigned int &r, const unsigned int &c) const {
    int gap = (DEF_HEIGHT - 1) / 2;
    double res = 0;
    for (int i =-gap; i < gap + 1; i++) {
        for (int j =  - gap; j < gap + 1; j++) {
            if (P[r + i][c + j].confidence != 0) { // if pixel is filled 
                res += P[r + i][c + j].confidence;
            }
        }
    }
    return res / (DEF_HEIGHT * DEF_WIDTH);
}

double map::compute_priority_patch(const unsigned int &r, const unsigned int &c) const {

    std::tuple<int, int> grad = gradient(r, c);
    std::tuple<double, double> normal = compute_normal(r, c);
    double D = abs(-std::get<1> (grad) * std::get<0> (normal) + std::get<0> (grad) * std::get<1> (normal)) / 255;
    double confid = compute_confidence(r, c);

    return confid * D;
}

bool map::viable_gradient(const unsigned int &r, const unsigned int &c) const {
    if ((r < 1) || (r >= rows - 1) || (c < 1) || (c >= cols - 1)) return false;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (P[r + i][c + j].confidence == 0) return false; // if pixel hasn't been filled yet
        }
    }
    return true;
}

bool map::patch_inside(const unsigned int &r, const unsigned int &c) const {
    int gap = (DEF_HEIGHT - 1) / 2;
    for (int i = r - gap; i < r + gap + 1; i++) {
        for (int j = c - gap; j < c + gap + 1; j++) {
            if (P[i][j].confidence != 1) return false; // stop condition : one of the pixels isn't Phi <=> pixel.confidence != 1
        }
    }
    return true;
}

// Vectorization gain here is limited: DEF_HEIGHT/DEF_WIDTH are typically small
// (patch size), and pixel** is an array of pointers (not contiguous memory),
// which restricts how well the compiler can vectorize across rows anyway.
double map::distance_from_patches(const unsigned int &r1, const unsigned int &c1, const unsigned int &r2, const unsigned int &c2) const {
    double sum = 0;
    int gap = (DEF_HEIGHT - 1) / 2;
    for (int i = -gap; i < gap + 1; i++) {
        #pragma omp simd reduction(+:sum)
        for (int j = -gap; j < gap + 1; j++) {
            if ((P[r1 + i][c1 + j].confidence != 0) && (P[r2 + i][c2 + j].confidence != 0)) {
                sum += pow((P[r1 + i][c1 + j].color_value - P[r2 + i][c2 + j].color_value), 2);
            }
        }
    }
    return sum;
} 

void map::fill_from_patch(const unsigned int &r1, const unsigned int &c1, const unsigned int &r2, const unsigned int &c2) {
    int gap = (DEF_HEIGHT - 1) / 2;

    P[r1][c1].confidence = compute_confidence(r1, c1); // updating confidence of the patch pixel with confidence value computed for priority
    P[r1][c1].color_value = P[r2][c2].color_value;

    for (int i = -gap; i < gap + 1; i++) {
        for (int j = -gap; j < gap + 1; j++) {
            // checks if the pixel of the destination patch is empty.
            if (P[r1 + i][c1 + j].confidence == 0) {
                // If it's the case, the color_value to the corresponding color_value of the other given patch
                P[r1 + i][c1 + j].color_value = P[r2 + i][c2 + j].color_value;
                // sets the confidence to the one of the (r1, c1)
                P[r1 + i][c1 + j].confidence = P[r1][c1].confidence;
            }
        }
    }
}

// Main hotspot of the whole inpainting algorithm: called once per filled pixel,
// scanning the entire image with a per-patch distance computation each time.
// Parallelized with a manual argmin reduction (OpenMP has no built-in reduction
// for "index of the minimum"): each thread tracks its own best candidate, merged
// at the end under a critical section (cheap, since it only runs once per thread,
// not once per pixel).
std::tuple<unsigned int, unsigned int> map::nearest_patch(const unsigned int &r, const unsigned int &c) const {
    unsigned int best_i = 0, best_j = 0;
    double best_distance = 10e6; // insanely high initial value, guaranteed to be beaten by any real first candidate

    int i_start = (DEF_HEIGHT + 1) / 2;
    int i_end = rows - DEF_HEIGHT + 2;
    int j_start = (DEF_WIDTH + 1) / 2;
    int j_end = cols - DEF_HEIGHT + 2;

    #pragma omp parallel
    {
        unsigned int local_best_i = 0, local_best_j = 0;
        double local_best_distance = 10e6;

        #pragma omp for collapse(2) schedule(dynamic)
        for (int i = i_start; i < i_end; i++) {
            for (int j = j_start; j < j_end; j++) {
                if (patch_inside(i, j)) {
                    double buffer = distance_from_patches(r, c, i, j);
                    if (buffer < local_best_distance) {
                        local_best_i = i;
                        local_best_j = j;
                        local_best_distance = buffer;
                    }
                }
            }
        }

        #pragma omp critical
        {
            if (local_best_distance < best_distance) {
                best_distance = local_best_distance;
                best_i = local_best_i;
                best_j = local_best_j;
            }
        }
    }

    return std::make_tuple(best_i, best_j);
}

// NOT parallelized: builds the polygon border sequentially from ordered
// coordinate pairs, each segment depending on the previous (r, c) state.
std::vector<std::tuple<std::tuple<unsigned int, unsigned int>, double>> map::Fill_Front(const std::vector<unsigned int>& coords) const {
    std::vector<std::tuple<std::tuple<unsigned int, unsigned int>, double>> res;
    //Construction of the border of the polygone to be filled
    unsigned int r = coords[0];
    unsigned int c = coords[1];
    double prio = compute_priority_patch(r,c);
    res.push_back(std::make_tuple(std::make_tuple(r,c), prio));
    unsigned int next_r = coords[coords.size()-2];
    unsigned int next_c = coords[coords.size()-1];
    for (unsigned int i = std::min(r,next_r); i <= std::max(r,next_r); i++) //As the last vertice is by convention vertical
        {
            prio = compute_priority_patch(i,c);
            res.push_back(std::make_tuple(std::make_tuple(i,c), prio));
        }
    for (unsigned int k = 2; k < coords.size(); k+= 2)
    {
        next_r = coords[k];
        next_c = coords[k+1];
        if (r == next_r && next_c > c)
        {
            for (unsigned int j = c+1; j <= next_c; j++)
            {
                prio = compute_priority_patch(r,j);
                res.push_back(std::make_tuple(std::make_tuple(r,c), prio));
            }
        }
        else if (r == next_r && next_c < c){
            for (unsigned int j = next_c; j < c; j++)
            {
                prio = compute_priority_patch(r,j);
                res.push_back(std::make_tuple(std::make_tuple(r,c), prio));
            }
        }
        else if (c == next_c && next_r > r){
            for (unsigned int i = r+1; i <= next_r; i++)
            {
                prio = compute_priority_patch(i,c);
                res.push_back(std::make_tuple(std::make_tuple(i,c), prio));
            }
        }
        else if (c == next_c && next_r < r){
            for (unsigned int i = next_r; i < r; i++)
            {
                prio = compute_priority_patch(i,c);
                res.push_back(std::make_tuple(std::make_tuple(i,c), prio));
            }
        }
        else{
            std::cout << "Points of the polygone should be on the same row or on the same column, and can not be consecutively equal" << std::endl;
        }
        r = next_r;
        c = next_c;
    }
    return res;
}

// NOT parallelized: mutates the shared frontline vector in place (push_back/erase),
// inherently sequential.
void map::update_frontline(std::vector<std::tuple<std::tuple<unsigned int, unsigned int>, double>> &frontline) {
    unsigned int i = std::get<0> (std::get<0> (frontline[frontline.size() - 1]));
    unsigned int j = std::get<1> (std::get<0> (frontline[frontline.size() - 1]));
    frontline.pop_back();
    int gap = (DEF_HEIGHT + 1) / 2;

    for (int k = 0; k < frontline.size(); k++) {
        for (int m = 0; m < gap; m++){
            for (int n = 0; n < gap; n++){
                if ((std::get<0> (frontline[frontline.size() - k - 1]) == std::make_tuple(i-m, j-n)) || 
                (std::get<0> (frontline[frontline.size() - k - 1]) == std::make_tuple(i-m, j+n)) || 
                (std::get<0> (frontline[frontline.size() - k - 1]) == std::make_tuple(i+m, j-n)) || 
                (std::get<0> (frontline[frontline.size() - k - 1]) == std::make_tuple(i+m, j+n))) {
                    frontline.erase(frontline.begin() + frontline.size() - k - 1);
                }
            }
        }
    }
    for (int k=-gap; k < gap+1; k++) {
        if (P[i-gap][j+k].confidence == 0) {
            frontline.push_back(std::make_tuple(std::make_tuple(i-gap, j+k), compute_priority_patch(i-gap, j+k)));
        }
        if (P[i+gap][j+k].confidence == 0) {
            frontline.push_back(std::make_tuple(std::make_tuple(i+gap, j+k), compute_priority_patch(i+gap, j+k)));
        }
    }
    for (int k=-gap+1; k < gap; k++)
    {
        if (P[i+k][j-gap].confidence == 0) {
            frontline.push_back(std::make_tuple(std::make_tuple(i+k, j-gap), compute_priority_patch(i+k, j-gap)));
        }
        if (P[i+k][j+gap].confidence == 0) {
            frontline.push_back(std::make_tuple(std::make_tuple(i+k, j+gap), compute_priority_patch(i+k, j+gap)));
        }
    }
}

map map::operator=(const map &pat) {
    if (this != &pat) { // prevents from destroying the object with a self assignment
        rows = pat.rows;
        cols = pat.cols;
        pixel ** buffer = new pixel *[rows];
        for (int i = 0; i < rows; i++) {
            buffer[i] = new pixel[cols];
            for (int j = 0; j < cols; j++) {
                buffer[i][j] = pat.P[i][j];
            }
            delete [] P[i];
        }
        delete [] P;
        P = buffer;
    }
    return *this;
}