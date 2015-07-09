
#ifndef __STEREO_VISION_HEADER__
#define __STEREO_VISION_HEADER__

#include <arm_math.h>
#include "main_parameters.h"

// Stereo vision header file
void stereo_vision_Kirk(uint8_t* in, q7_t* out, uint32_t image_width, uint32_t image_height, uint32_t disparity_range, uint8_t thr1, uint8_t thr2, uint8_t min_y, uint8_t max_y);
void stereo_vision(uint8_t* in, q7_t* out, uint32_t image_width, uint32_t image_height, uint32_t disparity_range, uint8_t thr1, uint8_t thr2, uint8_t min_y, uint8_t max_y);
void separate_image_line(uint8_t* in, q15_t* line1, q15_t* line2, uint32_t image_width);
void separate_image_line_offset(uint8_t* in, q15_t* line1, q15_t* line2, uint32_t image_width_bytes);
uint32_t evaluate_disparities(uint8_t* in, uint32_t image_width, uint32_t image_height, uint8_t disparity_threshold, uint32_t disparities_high);
void evaluate_disparities_altitude(uint8_t* in, uint32_t image_width, uint32_t image_height, uint8_t disparity_threshold, uint32_t* disparities, uint8_t altitude_levels, uint16_t x_min, uint16_t x_max, uint32_t* bad_pixels);
void evaluate_central_disparities(uint8_t* in, uint32_t image_width, uint32_t image_height, uint32_t* disparities, uint8_t n_disp_bins, uint8_t min_y, uint8_t max_y);
void evaluate_central_disparities2(uint8_t* in, uint32_t image_width, uint32_t image_height, uint32_t* disparities, uint8_t n_disp_bins, uint8_t min_y, uint8_t max_y, uint8_t disp_threshold);
void filter_disparity_map(uint8_t* in, uint8_t diff_threshold, uint32_t image_width, uint32_t image_height, uint8_t min_y, uint8_t max_y);
#endif
