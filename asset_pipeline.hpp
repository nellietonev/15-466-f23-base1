#pragma once
#include "PPU466.hpp"

/*
 * Asset Pipeline - Nellie Tonev
 *
 * Helper functions for runtime asset pipeline as well as authoring functions for binary assets.
 * Used to process PNG images into the palette table, background tiles, and sprites.
 */

/* Given a palette and a target color, returns the index in that palette at which the color can be found.
 * Returns an index out of range if the color is not found (during runtime, this is checked with an assertion) */
uint8_t get_index_in_palette(PPU466::Palette &palette, glm::u8vec4 color);

/* Given a data vector (typically generated from load_png) and a palette to pull color indices from,
 * generates a corresponding tile for the PPU466
 *
 * REQUIRES: the size of tile_data needs to be 8x8 (assertion fails otherwise), and all the colors in tile_data
 *          must be somewhere in the input palette */
PPU466::Tile generate_tile_from_data(std::vector< glm::u8vec4 > const &tile_data, PPU466::Palette palette);
