#include <array>
#include <fstream>
#include <vector>

#include "data_path.hpp"
#include "load_save_png.hpp"
#include "read_write_chunk.hpp"
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

std::vector< PPU466::Tile > generate_tiles_from_spritesheet(std::vector<glm::u8vec4> &spritesheet_data, glm::uvec2 &spritesheet_size, PPU466::Palette palette) {
    assert(spritesheet_size.x % 8 == 0 && spritesheet_size.y % 8 == 0);
    size_t rows = spritesheet_size.y / 8;
    size_t cols = spritesheet_size.x / 8;

    std::vector< PPU466::Tile > ret_vector;

    size_t row_scale = spritesheet_size.x * 8; /* of of pixels between each row of tiles in spritesheet */

    for (size_t r = rows; r > 0; r--) { /* current tile row in spritesheet, considering lower left */
        for (size_t c = 0; c < cols; c++) { /* current tile col in spritesheet */
            /* Extract the exact 8x8 pixel data to create the current tile */
            std::vector<glm::u8vec4> current_tile_data = std::vector<glm::u8vec4>(64);
            size_t tile_data_idx = 0;

            size_t start_idx = ((r - 1) * row_scale) + (c * 8); /* index in spritesheet_data of first pixel in current tile */

            for (size_t i = 0; i < 8; i++) {
                for (size_t j = 0; j < 8; j++) {
                    size_t cur_pixel = start_idx + (i * spritesheet_size.x) + j;
                    current_tile_data[tile_data_idx] = spritesheet_data[cur_pixel];
                    tile_data_idx++;
                }
            }
            assert(tile_data_idx == 64);

            /* Generate tile and add it to tile_table */
            PPU466::Tile current_spritesheet_tile = generate_tile_from_data(current_tile_data, palette);
            ret_vector.push_back(current_spritesheet_tile);
        }
    }

    return ret_vector;
}

void generate_level_layout_binary() {
    std::ofstream output_binary(data_path("assets/level-layout.bin"), std::ios::binary);

    /* Read in the level layout png */
    std::vector< glm::u8vec4 > level_layout_data;
    glm::uvec2 level_layout_size;
    load_png(data_path("assets/level-layout.png"), &level_layout_size, &level_layout_data, LowerLeftOrigin);
    assert(level_layout_size == glm::uvec2(32, 30));

    /* splitting data vector into 4 quadrants */
    std::array<std::string, 4> magic_values = {"Q_LL", "Q_LR", "Q_UL", "Q_UR"};

    size_t quadrant_height = level_layout_size.y / 2;
    size_t quadrant_width = level_layout_size.x / 2;

    for (uint8_t r = 0; r < 2; r++) { /* lower and upper quadrants */
        for (uint8_t c = 0; c < 2; c++) { /* left and right quadrants */
            std::vector< char > current_quadrant_data;
            size_t start_idx = (r * quadrant_height * level_layout_size.x) + (c * quadrant_width);

            /* reading a quadrant into current_quadrant_data */
            for (size_t i = 0; i < quadrant_height; i++) { /* current row of data */
                for (size_t j = 0; j < quadrant_width; j++) { /* current column of data */
                    size_t cur_pixel = start_idx + (i * level_layout_size.x) + j;

                    /* true if the corresponding pixel is black and false otherwise */
                    current_quadrant_data.push_back(level_layout_data[cur_pixel] == glm::u8vec4(0u) ? '0' : '1');
                }
            }
            assert(current_quadrant_data.size() == quadrant_height * quadrant_width);

            uint8_t q_idx = (r << 1) | c;
            write_chunk(magic_values[q_idx], current_quadrant_data, &output_binary);
        }
    }
}

