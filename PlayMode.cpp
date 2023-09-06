#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <glm/gtx/hash.hpp>
#include <unordered_set>

#include "data_path.hpp"
#include "load_save_png.hpp"


uint8_t get_index_in_palette(PPU466::Palette &palette, glm::u8vec4 color) {
    for (uint8_t i = 0; i < 4; i++) {
        if (palette[i] == color) return i;
    }
    return 4;
}

PPU466::Tile PlayMode::generate_tile_from_png(std::string const &suffix, PaletteTableIndex paletteTableIndex) {
    std::vector< glm::u8vec4 > tile_data;
    glm::uvec2 tile_size;
    load_png(data_path(suffix), &tile_size, &tile_data, LowerLeftOrigin);
    assert(tile_size == glm::uvec2(8, 8));

    PPU466::Tile return_tile = {};
    for (size_t i = 0; i < 8; i++) { /* index into bit0 and bit1 */
        uint8_t bit0_row = 0;
        uint8_t bit1_row = 0;

        for (size_t j = 0; j < 8; j++) { /* specific bit within the each uint8_t */
            /* set the bit0 and bit1 rows */
            glm::u8vec4 current_pixel_color = tile_data[(i * 8) + j];

            uint8_t current_pixel_color_index = get_index_in_palette(ppu.palette_table[paletteTableIndex],
                                                                     current_pixel_color);
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

PlayMode::PlayMode() {
    // TODO: call the asset pipeline here to load in the background tiles and sprites

    /* Clear & reset tile data */
    ppu.background = {};
    ppu.palette_table = {};
    ppu.tile_table = {};

    { /* Populating the PPU's palette table
       * Uses palette_table_data.png as a storage format for all 8 palettes in palette table
       * Inspired by Matei Budiu's implementation of individual palettes as their own 2x2 png files */
        std::vector<glm::u8vec4> palette_table_data;
        glm::uvec2 palette_table_size;
        load_png(data_path("assets/palette_table_data.png"), &palette_table_size, &palette_table_data, UpperLeftOrigin);
        assert(palette_table_size == glm::uvec2(4, 8));

        for (size_t i = 0; i < 8; i++) { // looping over each palette in table
            size_t current_start_idx = 4 * i;

            PPU466::Palette current_palette = {};
            for (size_t j = 0; j < 4; j++) { // looping over each color in palette
                current_palette[j] = palette_table_data[current_start_idx + j];
            }

            ppu.palette_table[i] = current_palette;
        }
    }

    /* Setting the background color to same color as ground's primary color */
    ppu.background_color = ppu.palette_table[GroundPalette][2];

    /* Loading in the tile for the default ground tile */
    PPU466::Tile ground_tile = generate_tile_from_png("assets/ground.png", GroundPalette);
    ppu.tile_table[0] = ground_tile;

    for (size_t i = 0; i < ppu.background.size(); i++)  {
        // ppu.background[i] |= PlayerPalette << 8;
        // keep this in mind for the other tiles in the background later
    }

    //use sprite 32 as a "player":
    PPU466::Tile player_tile = generate_tile_from_png("assets/bee-default.png", PlayerPalette);
    ppu.tile_table[32] = player_tile;

    //player sprite:
    player_at = glm::vec2(20.0f, 225.0f);
    ppu.sprites[0].index = 32;
    ppu.sprites[0].attributes = PlayerPalette;
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
    // TODO: Add an interaction key and a quitting key

    if (evt.type == SDL_KEYDOWN) {
        if (evt.key.keysym.sym == SDLK_LEFT) {
            left.downs += 1;
            left.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_RIGHT) {
            right.downs += 1;
            right.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_UP) {
            up.downs += 1;
            up.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_DOWN) {
            down.downs += 1;
            down.pressed = true;
            return true;
        }
    } else if (evt.type == SDL_KEYUP) {
        if (evt.key.keysym.sym == SDLK_LEFT) {
            left.pressed = false;
            return true;
        } else if (evt.key.keysym.sym == SDLK_RIGHT) {
            right.pressed = false;
            return true;
        } else if (evt.key.keysym.sym == SDLK_UP) {
            up.pressed = false;
            return true;
        } else if (evt.key.keysym.sym == SDLK_DOWN) {
            down.pressed = false;
            return true;
        }
    }

    return false;
}

void PlayMode::update(float elapsed) {
    // TODO: check the interaction key and update variables / game state
    constexpr float PlayerSpeed = 30.0f;
    if (left.pressed) player_at.x -= PlayerSpeed * elapsed;
    if (right.pressed) player_at.x += PlayerSpeed * elapsed;
    if (down.pressed) player_at.y -= PlayerSpeed * elapsed;
    if (up.pressed) player_at.y += PlayerSpeed * elapsed;

    //reset button press counters:
    left.downs = 0;
    right.downs = 0;
    up.downs = 0;
    down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
    //--- set ppu state based on game state ---

    //player sprite:
    ppu.sprites[0].x = int8_t(player_at.x);
    ppu.sprites[0].y = int8_t(player_at.y);

    //--- actually draw ---
    ppu.draw(drawable_size);
}
