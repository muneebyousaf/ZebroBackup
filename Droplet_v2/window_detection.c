/*
 * window_detection.c
 *
 *  Created on: Sep 9, 2013
 *      Author: mavlab
 */

#define RES 100

#include "window_detection.h"
#include "usart.h"

uint16_t detect_window_sizes(uint8_t* in, uint32_t image_width, uint32_t image_height, uint16_t* coordinate, uint32_t* integral_image, uint8_t MODE)
{
		// whether to calculate the integral image (only do once):
		uint8_t calculate_integral_image = 1;
		// whether the algorithm will determine the size of the window on the basis of the average distance to objects in view
		uint8_t determine_size = 0;
		uint16_t sizes[1];
		uint16_t min_response[4];
		uint8_t min_index = 0;
		uint8_t min_xc = 0;
		uint8_t min_yc = 0;
		uint8_t s = 0;
		sizes[0] = 30; sizes[1] = 40; sizes[2] = 50; sizes[3] = 60;
		//sizes[0] = 40; //sizes[1] = 53; sizes[2] = 65;
		for(s = 0; s < 4; s++)
		{
			// coordinate will contain the coordinate, min_response will be the best match * 100
			calculate_integral_image = (s==0);// only calculate the integal image for the first window size
			min_response[s] = detect_window(in, image_width, image_height, coordinate, determine_size, &sizes[s], calculate_integral_image, integral_image, MODE);
			if(s == 0 || min_response[s] < min_response[s-1])
			{
				min_index = s;
				min_xc = coordinate[0];
				min_yc = coordinate[1];
			}
		}

		coordinate[0] = min_xc;
		coordinate[1] = min_yc;
		return min_response[min_index];
}

uint16_t detect_window(uint8_t* in, uint32_t image_width, uint32_t image_height, uint16_t* coordinate, uint8_t determine_size, uint16_t* size, uint8_t calculate_integral_image, uint32_t* integral_image, uint8_t MODE)
{
	/*
	 * Steps:
	 * (0) filter out the bad pixels (i.e., those lower than 4) and replace them with a disparity of 6.
	 * (1) get integral image (if calculate_integral_image == 1)
	 * (2) get average disparity to determine probable size (if determine_size == 1)
	 * (3) determine responses per location while determining the best-matching location (put it in coordinate)
	 */

	// output of the function:
	uint16_t min_response = 1000;

	// parameters:
	uint16_t image_border = 10;
	uint16_t window_size, border_size, feature_size, px_whole, px_inner, px_border, px_outer;
	uint16_t relative_border = 15; // border in percentage of window size

	// declaration other vars:
	uint16_t x, y;
	uint32_t response;

	if(MODE == MODE_DISPARITY)
	{
		// (0) filter the bad pixels out, replacing them with 6:
		filter_bad_pixels(in, image_width, image_height);
	}
	/*else if(MODE == MODE_ILLUMINANCE)
	{
		// reduce the range of illuminance
		uint8_t n_bits = 2;
		transform_illuminance_image(in, image_width, image_height, n_bits);
	}*/

	// (1) get integral image (if calculate_integral_image == 1)
	if(calculate_integral_image)
	{
		get_integral_image(in, image_width, image_height, integral_image);
	}

	// (2) get average disparity to determine probable size (can also be given from the outside)
	if(determine_size)
	{
		// get average disparity:
		uint32_t avg = get_avg_disparity(image_border, image_border, image_width - image_border, image_height-image_border, integral_image, image_width, image_height);
		//		disp: window size
		//		4.0: 30 x 30
		//		4.5: 40 x 40
		//		5.0: 50 x 50
		if(avg / 10 < 42)
		{
			*size = 30;
		}
		else if(avg / 10 < 48)
		{
			*size = 40;
		}
		else
		{
			*size = 50;
		}
	}
	// window size is without border, feature size is with border:
	window_size = *size;
	border_size = (relative_border * window_size) / RES;
	feature_size = window_size + border_size;
	px_inner = feature_size-2*border_size;
	px_inner = px_inner * px_inner;
	px_whole = feature_size * feature_size;
	px_border = px_whole - px_inner;
	px_outer = border_size * window_size; 

	//print_number(px_inner, 0); print_space();

	// (3) determine a response map for that size
	for(x = 0; x < image_width-feature_size; x++)
	{
		for(y = 0; y < image_height-feature_size; y++)
		{
			response = get_window_response(x, y, feature_size, border_size, integral_image, image_width, image_height, px_inner, px_border);

			if(response < RES)
			{
				// the inside is further away than the outside, perform the border test:
				response = get_border_response(x, y, feature_size, window_size, border_size, integral_image, image_width, image_height, px_inner, px_outer);

				//in[x+y*image_width] = response;

				if(response < min_response)
				{
					coordinate[0] = x;
					coordinate[1] = y;
					min_response = response;
				}
			}
			else
			{
				//in[x+y*image_width] = 255;
			}
		}
	}

	//in[coordinate[0]+coordinate[1]*image_width] = 255;

	// the coordinate is at the top left corner of the feature,
	// the center of the window is then at:
	coordinate[0] += feature_size / 2;
	coordinate[1] += feature_size / 2;

	return min_response;
}

// this function can help if the window is not visible anymore:
uint16_t detect_escape(uint8_t* in, uint32_t image_width, uint32_t image_height, uint16_t* escape_coordinate, uint32_t* integral_image, uint8_t n_cells)
{
	uint8_t c, r, min_c, min_r;
	uint16_t cell_width, cell_height;
	uint32_t min_avg = 10000;
	uint32_t avg;
	uint16_t border = 10;
	cell_width = (image_width-2*border) / n_cells;
	cell_height = (image_height-2*border) / n_cells;
	// Get the average disparities of all cells in a grid:
	for (c = 0; c < n_cells; c++)
	{
		for(r = 0; r < n_cells; r++)
		{
			avg = get_avg_disparity(c*cell_width+border, r*cell_height+border, (c+1)*cell_width+border, (r+1)*cell_height+border, integral_image, image_width, image_height);
			if(avg < min_avg)
			{
				min_avg = avg;
				min_c = c;
				min_r = r;
			}
		}
	}
	// return coordinates for the best option:
	if(min_avg == 10000)
	{
		escape_coordinate[0] = image_width / 2;
		escape_coordinate[1] = image_height / 2;
	}
	else
	{
		escape_coordinate[0] = min_c * cell_width + border + cell_width / 2;
		escape_coordinate[1] = min_r * cell_height + border + cell_height / 2;
	}

	return min_avg;
}

void get_integral_image(uint8_t* in, uint32_t image_width, uint32_t image_height, uint32_t* integral_image)
{
	uint16_t x, y;
	for(x = 0; x < image_width; x++)
	{
		for(y = 0; y < image_height; y++)
		{
			if(x >= 1 && y >= 1)
			{
				integral_image[x+y*image_width] = (uint32_t) in[x+y*image_width] + integral_image[x-1+y*image_width] + integral_image[x+(y-1)*image_width] - integral_image[x-1+(y-1)*image_width];
			}
			else if(x >= 1)
			{
				integral_image[x+y*image_width] = (uint32_t) in[x+y*image_width] + integral_image[x-1+y*image_width];
			}
			else if(y >= 1)
			{
				integral_image[x+y*image_width] = (uint32_t) in[x+y*image_width] + integral_image[x+(y-1)*image_width];
			}
			else
			{
				integral_image[x+y*image_width] = (uint32_t) in[x+y*image_width];
			}
		}
	}
}

uint32_t get_sum_disparities(uint16_t min_x, uint16_t min_y, uint16_t max_x, uint16_t max_y, uint32_t* integral_image, uint32_t image_width, uint32_t image_height)
{
	uint32_t sum;
	sum = integral_image[min_x + min_y * image_width] + integral_image[max_x + max_y * image_width] - integral_image[max_x + min_y * image_width] - integral_image[min_x + max_y * image_width];
	return sum;
}

uint32_t get_avg_disparity(uint16_t min_x, uint16_t min_y, uint16_t max_x, uint16_t max_y, uint32_t* integral_image, uint32_t image_width, uint32_t image_height)
{
	uint16_t w, h;
	uint32_t sum, avg, n_pix;

	// width and height of the window
	w = max_x - min_x + 1;
	h = max_y - min_y + 1;
	n_pix = w * h;
	// sum over the area:
	sum = integral_image[min_x + min_y * image_width] + integral_image[max_x + max_y * image_width] - integral_image[max_x + min_y * image_width] - integral_image[min_x + max_y * image_width];
	// take the average, scaled by RES:
	avg = (sum * RES) / n_pix;
	return avg;
}


uint16_t get_window_response(uint16_t x, uint16_t y, uint16_t feature_size, uint16_t border, uint32_t* integral_image, uint16_t image_width, uint16_t image_height, uint16_t px_inner, uint16_t px_border)
{
	uint16_t whole_area, inner_area, resp;
	whole_area = get_sum_disparities(x, y, x+feature_size, y+feature_size, integral_image, image_width, image_height);
	inner_area = get_sum_disparities(x+border, y+border, x+feature_size-border, y+feature_size-border, integral_image, image_width, image_height);
	resp =  (RES*((inner_area*RES) / px_inner)) / (((whole_area - inner_area)*RES) / px_border );

	/*print_number(whole_area, 0);
	print_space();
	print_number(inner_area, 0);
	print_space();
	print_number(resp, 1);*/

	return resp;
}

uint16_t get_border_response(uint16_t x, uint16_t y, uint16_t feature_size, uint16_t window_size, uint16_t border, uint32_t* integral_image, uint16_t image_width, uint16_t image_height, uint16_t px_inner, uint16_t px_outer)
{
	uint16_t inner_area, avg_inner, left_area, right_area, up_area, down_area, darkest, avg_dark, resp;
	// inner area
	inner_area = get_sum_disparities(x+border, y+border, x+feature_size-border, y+feature_size-border, integral_image, image_width, image_height);
	avg_inner = RES*inner_area / px_inner;
	// outer areas:
	left_area = get_sum_disparities(x, y+border, x+border, y+border+window_size, integral_image, image_width, image_height);
	right_area = get_sum_disparities(x+border+window_size, y+border, x+2*border+window_size, y+border+window_size, integral_image, image_width, image_height);
	up_area = get_sum_disparities(x+border, y, x+border+window_size, y+border, integral_image, image_width, image_height);
	down_area = get_sum_disparities(x+border, y+border+window_size, x+border+window_size, y+2*border+window_size, integral_image, image_width, image_height);
	// darkest outer area:
	darkest = (left_area < right_area) ? left_area : right_area;
	darkest = (darkest < up_area) ? darkest : up_area;
	darkest = (darkest < down_area) ? darkest : down_area;
	avg_dark = RES*darkest / px_outer;
	if(avg_dark < avg_inner)
		resp = RES;
	else
		resp = RES*avg_inner / avg_dark;

	/*
	print_number(avg_inner, 0);
	print_space();
	print_number(avg_dark, 0);
	print_space();
	print_number(resp, 1);
	 */
	return resp;
}

void filter_bad_pixels(uint8_t* in, uint32_t image_width, uint32_t image_height)
{
	uint16_t x, y;
	for(x = 0; x < image_width; x++)
	{
		for(y = 0; y < image_height; y++)
		{
			if(in[x+y*image_width] < 4)
			{
				in[x+y*image_width] = 6;
			}
		}
	}
}


void transform_illuminance_image(uint8_t* in, uint8_t* out, uint32_t image_width, uint32_t image_height, uint8_t n_bits, uint8_t bright_win)
{
	uint16_t x, y;
	for(x = 0; x < image_width; x++)
	{
		for(y = 0; y < image_height; y++)
		{
			// we put the right image entirely in the left image instead of in the even rows:
			if(!bright_win)
			{
				out[x+y*image_width] = in[2*(x+y*image_width)] >> n_bits;
			}
			else
			{
				out[x+y*image_width] = (255 - in[2*(x+y*image_width)]) >> n_bits;
			}
		}
	}
}
