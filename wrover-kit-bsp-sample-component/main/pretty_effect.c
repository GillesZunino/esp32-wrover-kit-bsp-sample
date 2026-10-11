/*
   This code generates an effect that should pass the 'fancy graphics' qualification
   as set in the comment in the spi_master code.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <math.h>

#include "decode_image.h"
#include "pretty_effect.h"


#define SIN_LUT_SIZE 1024
#define SIN_LUT_MASK (SIN_LUT_SIZE - 1)

static int8_t sin_lut[SIN_LUT_SIZE];

uint16_t *pixels;

// Grab a rgb16 pixel from the esp32_tiles image
static inline uint16_t get_bgnd_pixel(int x, int y)
{
    // Clamp coordinates to valid image bounds
    x = (x < 0) ? 0 : (x >= IMAGE_W) ? IMAGE_W - 1 : x;
    y = (y < 0) ? 0 : (y >= IMAGE_H) ? IMAGE_H - 1 : y;

    // Get color of the pixel on x,y coords
    return (uint16_t) * (pixels + (y * IMAGE_W) + x);
}

// This variable is used to detect the next frame.
static int prev_frame = -1;

// Instead of calculating the offsets for each pixel we grab, we pre-calculate the values whenever a frame changes, then reuse
// these as we go through all the pixels in the frame. This is much, much faster.
static int8_t xofs[320], yofs[240];
static int8_t xcomp[320], ycomp[240];

// Calculate the pixel data for a set of lines (with implied line size of 320). Pixels go in dest, line is the Y-coordinate of the
// first line to be calculated, linect is the amount of lines to calculate. Frame increases by one every time the entire image
// is displayed; this is used to go to the next frame of animation.
void pretty_effect_calc_lines(uint16_t *dest, int line, int frame, int linect)
{
    if (frame != prev_frame) {
        // Integer phase increments derived from round(coeff * SIN_LUT_SIZE / 2π)
        {
            uint32_t phase = ((uint32_t)frame * 24u) & SIN_LUT_MASK;
            for (int x = 0; x < 320; x++) {
                xofs[x] = sin_lut[phase];
                phase = (phase + 10u) & SIN_LUT_MASK;
            }
        }
        {
            uint32_t phase = ((uint32_t)frame * 16u) & SIN_LUT_MASK;
            for (int y = 0; y < 240; y++) {
                yofs[y] = sin_lut[phase];
                phase = (phase + 8u) & SIN_LUT_MASK;
            }
        }
        {
            uint32_t phase = ((uint32_t)frame * 18u) & SIN_LUT_MASK;
            for (int x = 0; x < 320; x++) {
                xcomp[x] = sin_lut[phase];
                phase = (phase + 20u) & SIN_LUT_MASK;
            }
        }
        {
            uint32_t phase = ((uint32_t)frame * 11u) & SIN_LUT_MASK;
            for (int y = 0; y < 240; y++) {
                ycomp[y] = sin_lut[phase];
                phase = (phase + 24u) & SIN_LUT_MASK;
            }
        }
        prev_frame = frame;
    }
    for (int y = line; y < line + linect; y++) {
        for (int x = 0; x < 320; x++) {
            *dest++ = get_bgnd_pixel(x + yofs[y] + xcomp[x], y + xofs[x] + ycomp[y]);
        }
    }
}

esp_err_t pretty_effect_init(void)
{
    // Floating point operations are notoriously slow on ESP32 - Pre-calculate all the values of sin() we will need and store them in a lookup table
    for (int i = 0; i < SIN_LUT_SIZE; i++) {
        sin_lut[i] = (int8_t)roundf(sinf(2.0f * (float)M_PI * i / SIN_LUT_SIZE) * 4.0f);
    }

    return decode_image(&pixels);
}
