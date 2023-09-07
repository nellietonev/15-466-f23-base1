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

#include "asset_pipeline.hpp"


PlayMode::PlayMode() {
    {  /* (1) Clear & reset tile data */
        ppu.background = {};
        ppu.palette_table = {};
        ppu.tile_table = {};
    }

    { /* (2) Populating the PPU's palette table
       * Uses palette_table_data.png as a storage format for all 8 palettes in palette table
       * Inspired by Matei Budiu's implementation of individual palettes as their own 2x2 png files
       * Palette colors are taken from Aesprite default RGB palette (at least for now until I look for other colors) */
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

        /* Setting the background color to same color as ground's primary color */
        ppu.background_color = ppu.palette_table[GroundPalette][2];
    }

    { /* (3) Processing png spritesheets for all the background tiles */
        {/* Default ground tile */
            std::vector<glm::u8vec4> ground_data;
            glm::uvec2 ground_size;
            load_png(data_path("assets/ground.png"), &ground_size, &ground_data, LowerLeftOrigin);
            assert(ground_size == glm::uvec2(8, 8));

            PPU466::Tile ground_tile = generate_tile_from_data(ground_data, ppu.palette_table[GroundPalette]);
            ppu.tile_table[0] = ground_tile;
        }

        {/* 8 distinct tiles for all the maze edges
          * Stored in tile_table indices 1-4 and 6-9 (inclusive), as 5 corresponds to a blank tile in spritesheet */
            std::vector<glm::u8vec4> maze_data;
            glm::uvec2 maze_size;
            load_png(data_path("assets/maze-tiles.png"), &maze_size, &maze_data, LowerLeftOrigin);
            assert(maze_size == glm::uvec2(24, 24));

            size_t current_tile_table_idx = 1;
            for (size_t r = 3; r > 0; r--) { /* current tile row in spritesheet, considering lower left */
                for (size_t c = 0; c < 3; c++) { /* current tile col in spritesheet */
                    /* Extract the exact 8x8 pixel data to create the current tile */
                    std::vector<glm::u8vec4> current_tile_data = std::vector<glm::u8vec4>(64);
                    size_t tile_data_idx = 0;

                    size_t row_scale = 24 * 8; /* number of pixels in maze_data between each row of tiles in spritesheet */
                    size_t start_idx = ((r - 1) * row_scale) + (c * 8); /* index in maze_data of first pixel in current tile */

                    for (size_t i = 0; i < 8; i++) {
                        for (size_t j = 0; j < 8; j++) {
                            size_t cur_pixel = start_idx + (i * 24) + j;
                            current_tile_data[tile_data_idx] = maze_data[cur_pixel];
                            tile_data_idx++;
                        }
                    }
                    assert(tile_data_idx == 64);

                    /* Generate tile and add it to tile_table */
                    PPU466::Tile current_maze_tile = generate_tile_from_data(current_tile_data, ppu.palette_table[MazeLitPalette]);
                    ppu.tile_table[current_tile_table_idx] = current_maze_tile;
                    current_tile_table_idx++;
                }
            }
        }
    }

    { /* (4) Setting up background tiles and palettes */
        // load in the sheet that deals with

        // TODO: delete this test
        ppu.background[129] = 7 | MazeUnlitPalette << 8;
        ppu.background[130] = 8 | MazeUnlitPalette << 8;
        ppu.background[131] = 9 | MazeUnlitPalette << 8;
        ppu.background[193] = 4 | MazeUnlitPalette << 8;
        ppu.background[195] = 6 | MazeUnlitPalette << 8;
        ppu.background[257] = 1 | MazeUnlitPalette << 8;
        ppu.background[258] = 2 | MazeUnlitPalette << 8;
        ppu.background[259] = 3 | MazeUnlitPalette << 8;

        for (size_t i = 0; i < ppu.background.size(); i++) {
            // ppu.background[i] |= PlayerPalette << 8;
            // keep this in mind for the other tiles in the background later
        }
    }

    { /* Player Sprite */
        //use sprite 32 as a "player":
        std::vector<glm::u8vec4> player_data;
        glm::uvec2 player_size;
        load_png(data_path("assets/bee-default.png"), &player_size, &player_data, LowerLeftOrigin);
        PPU466::Tile player_tile = generate_tile_from_data(player_data, ppu.palette_table[PlayerPalette]);
        ppu.tile_table[32] = player_tile;

        //player sprite:
        player_at = glm::vec2(20.0f, 225.0f); /* starting game position */
        ppu.sprites[0].index = 32;
        ppu.sprites[0].attributes = PlayerPalette;
    }
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
