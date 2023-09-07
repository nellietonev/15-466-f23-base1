#pragma once
#include "PPU466.hpp"

/*
 * Asset Pipeline - Nellie Tonev
 *
 * Helper functions for runtime asset pipeline as well as authoring functions for binary assets.
 * Used to process PNG images into the palette table, background tiles, and sprites.
 * Also used to convert black-and-white pixel map of level into useful binary chunks for runtime processing.
 */

/* Given a palette and a target color, returns the index in that palette at which the color can be found.
 * Returns an index out of range if the color is not found (during runtime, this is checked with an assertion)
 */
uint8_t get_index_in_palette(PPU466::Palette &palette, glm::u8vec4 color);

/* Given a data vector (typically generated from load_png) and a palette to pull color indices from,
 * generates a corresponding tile for the PPU466.
 * For accurate results, have data loaded with LowerLeftOrigin.
 *
 * REQUIRES: the size of tile_data needs to be 8x8 (assertion fails otherwise), all the colors in tile_data
 *          must be somewhere in the input palette
 */
PPU466::Tile generate_tile_from_data(std::vector< glm::u8vec4 > const &tile_data, PPU466::Palette palette);

/*
 * Given pixel color data and the size of a spritesheet, returns a vector of PPU466 tiles corresponding to slicing the
 * spritesheet into 8x8 tiles.
 * For accurate results, have spritesheet data loaded with LowerLeftOrigin.
 *
 * REQUIRES: The width and height of the spritesheet must both be divisible by 8, and all colors in spritesheet_data
 *          must be found somewhere in the input palette.
 */
std::vector< PPU466::Tile > generate_tiles_from_spritesheet(std::vector<glm::u8vec4> &spritesheet_data,
                                                            glm::uvec2 &spritesheet_size,
                                                            PPU466::Palette palette);

/*
 * This function is run as part of the authoring process, not during runtime.
 *
 * Takes a level layout png image (transparent png with black pixels corresponding to a tile on the background)
 * and then processes it into 4 chunks of chars (for 4 quadrants of the background, which are relevant to the geme mechanic of illuminating
 * the maze, quadrants at a time). In the chunks, '1' indicates there is a maze wall at that location on screen, while
 * '0' means there is just ground. Chunks are stored in `dist/assets/level-layout.bin`
 */
void generate_level_layout_binary();