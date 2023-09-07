#include <vector>
#include "PPU466.hpp"

uint8_t get_index_in_palette(PPU466::Palette &palette, glm::u8vec4 color) {
    for (uint8_t i = 0; i < 4; i++) {
        if (palette[i] == color) return i;
    }

    /* asserts should check that this doesn't happen */
    return 4;
}

PPU466::Tile generate_tile_from_data(std::vector<glm::u8vec4> const &tile_data, PPU466::Palette palette) {
    assert(tile_data.size() == 64);
    PPU466::Tile return_tile = {};
    for (size_t i = 0; i < 8; i++) { /* index into bit0 and bit1 */
        uint8_t bit0_row = 0;
        uint8_t bit1_row = 0;

        for (size_t j = 0; j < 8; j++) { /* specific bit within the each uint8_t */
            /* set the bit0 and bit1 rows */
            glm::u8vec4 current_pixel_color = tile_data[(i * 8) + j];

            uint8_t current_pixel_color_index = get_index_in_palette(palette, current_pixel_color);
            assert(current_pixel_color_index < 4);

            uint8_t current_bit0_bit = (current_pixel_color_index % 2 == 0) ? 0 : 1;
            uint8_t current_bit1_bit = (current_pixel_color_index < 2) ? 0 : 1;

            bit0_row |= (current_bit0_bit << j);
            bit1_row |= (current_bit1_bit << j);
        }

        return_tile.bit0[i] = bit0_row;
        return_tile.bit1[i] = bit1_row;
    }

    return return_tile;
}