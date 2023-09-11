#include <fstream>
#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <unordered_set>

#include "data_path.hpp"
#include "load_save_png.hpp"
#include "read_write_chunk.hpp"

#include "asset_pipeline.hpp"


void PlayMode::illuminate_quadrant(uint8_t quadrant) {
    for (size_t i = 0; i < quadrant_height; i++) {
        for (size_t j = 0; j < quadrant_width; j++) {
            size_t quadrant_pixel = (i * quadrant_width) + j;
            size_t background_pixel = start_idxs[quadrant] + (i * PPU466::BackgroundWidth) + j;

            if (quadrant_chunks[quadrant][quadrant_pixel] == '1') {
                /* clear the palette bits to be 0 (preserve the tile index bits) */
                ppu.background[background_pixel] &= 0b11111111;

                /* swap the palette */
                ppu.background[background_pixel] |= MazeLitPalette << 8;
            }
        }
    }
}

void PlayMode::draw_quadrant(uint8_t quadrant) {
    for (size_t i = 0; i < quadrant_height; i++) {
        for (size_t j = 0; j < quadrant_width; j++) {
            size_t quadrant_pixel = (i * quadrant_width) + j;
            size_t background_pixel = start_idxs[quadrant] + (i * PPU466::BackgroundWidth) + j;

            auto has_maze_tile_Up = [this, &quadrant, &quadrant_pixel]() {
                return quadrant_chunks[quadrant][quadrant_pixel - quadrant_width] == '1';
            };
            auto has_maze_tile_Down = [this, &quadrant, &quadrant_pixel]() {
                return quadrant_chunks[quadrant][quadrant_pixel + quadrant_width] == '1';
            };
            auto has_maze_tile_Left = [this, &quadrant, &quadrant_pixel]() {
                return quadrant_chunks[quadrant][quadrant_pixel - 1] == '1';
            };
            auto has_maze_tile_Right = [this, &quadrant, &quadrant_pixel]() {
                return quadrant_chunks[quadrant][quadrant_pixel + 1] == '1';
            };

            if (quadrant_chunks[quadrant][quadrant_pixel] == '1') {
                /* based on the surrounding tiles, determines which maze tile to draw
                 * currently, this implementation doesn't check across the quadrant borders, so those will all have edges
                 *      even if they shouldn't but it's good enough for now
                 * Also the edge cases are a little bit hard-coded and not good at the moment */
                uint8_t tile_table_idx;

                /* edges of quadrant, kind of hard-codey*/
                if (i == 0 || i == quadrant_height - 1 || j == 0 || j == quadrant_width - 1) {
                    if (i == 0) { /* top edge of quadrant */
                        if (j == 0 || !(has_maze_tile_Left())) tile_table_idx = 7;
                        else if (j == quadrant_width - 1 || !(has_maze_tile_Right())) tile_table_idx = 9;
                        else tile_table_idx = 8;
                    }
                    else if (i == quadrant_height - 1) { /* bottom edge of quadrant */
                        if (j == 0 || !(has_maze_tile_Left())) tile_table_idx = 1;
                        else if (j == quadrant_width - 1 || !(has_maze_tile_Right())) tile_table_idx = 3;
                        else tile_table_idx = 2;
                    }
                    else if (j == 0) { /* left edge of quadrant */
                        if (!(has_maze_tile_Up())) tile_table_idx = 7;
                        else if (!(has_maze_tile_Down())) tile_table_idx = 1;
                        else tile_table_idx = 4;
                    }
                    else { /* right edge of quadrant */
                        if (!(has_maze_tile_Up())) tile_table_idx = 9;
                        else if (!(has_maze_tile_Down())) tile_table_idx = 3;
                        else tile_table_idx = 6;
                    }
                }
                else { /* not corners, still not very pretty code, but it works so it's good enough, yay stupid code */
                    if (has_maze_tile_Down() && has_maze_tile_Up() && has_maze_tile_Left() && has_maze_tile_Right()) tile_table_idx = 5;
                    else if (has_maze_tile_Up() && has_maze_tile_Down()) {
                        /* just left or just right */
                        if (has_maze_tile_Right()) tile_table_idx = 4;
                        else tile_table_idx = 6;
                    }
                    else if (has_maze_tile_Up()) {
                        /* One of the bottom ones, 1-3 */
                        if (has_maze_tile_Left() && has_maze_tile_Right()) tile_table_idx = 2;
                        else if (has_maze_tile_Right()) tile_table_idx = 1;
                        else  tile_table_idx = 3;
                    }
                    else {
                        /* One of the top ones, 7-9 */
                        if (has_maze_tile_Left() && has_maze_tile_Right()) tile_table_idx = 8;
                        else if (has_maze_tile_Right()) tile_table_idx = 7;
                        else tile_table_idx = 9;
                    }
                }

                /* set the maze tile */
                ppu.background[background_pixel] = tile_table_idx | MazeUnlitPalette << 8;
            }
            else {
                ppu.background[background_pixel] = 0 | GroundPalette << 8;
            }
        }
    }
}

PlayMode::PlayMode() {
    ppu.background = {};
    ppu.palette_table = {};
    ppu.tile_table = {};

    { /* (1) Populating the PPU's palette table
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

    { /* (2) Processing png spritesheets for all the background tiles */
        {/* Default ground tile (non-maze) */
            std::vector<glm::u8vec4> ground_data;
            glm::uvec2 ground_size;
            load_png(data_path("assets/ground.png"), &ground_size, &ground_data, LowerLeftOrigin);
            assert(ground_size == glm::uvec2(8, 8));

            PPU466::Tile ground_tile = generate_tile_from_data(ground_data, ppu.palette_table[GroundPalette]);
            ppu.tile_table[0] = ground_tile;

            for (size_t i = 0; i < ppu.background.size(); i++) {
                ppu.background[i] = 0 | GroundPalette << 8;
            }
        }

        {/* 8 distinct tiles for all the maze edges
          * Stored in tile_table indices 1-9 (inclusive) */
            std::vector<glm::u8vec4> maze_data;
            glm::uvec2 maze_size;
            load_png(data_path("assets/maze-tiles.png"), &maze_size, &maze_data, LowerLeftOrigin);
            assert(maze_size == glm::uvec2(24, 24));

            std::vector< PPU466::Tile > maze_tiles = generate_tiles_from_spritesheet(maze_data, maze_size,ppu.palette_table[MazeLitPalette]);

            /* Copy over generated maze tiles into the tile table */
            size_t current_tile_table_idx = 1;
            for (PPU466::Tile const &maze_tile : maze_tiles) {
                ppu.tile_table[current_tile_table_idx] = maze_tile;
                current_tile_table_idx++;
            }
        }
    }

    { /* (3) Setting up background tiles and their palettes using level layout binary */
        /* Read the chunks for all 4 quadrants and finding their start pixels in background */
        std::ifstream level_layout_binary(data_path("assets/level-layout.bin"), std::ios::binary);
        for (uint8_t i = 0; i < 4; i++) {
            read_chunk(level_layout_binary, magic_values[i], &quadrant_chunks[i]);

            /* row and column of quadrants */
            uint8_t r = i >> 1;
            uint8_t c = i & 1;

            start_idxs[i] = (r * quadrant_height * PPU466::BackgroundWidth) + (c * quadrant_width);
        }

        /* Draw each quadrant, initially unlit */
        for (uint8_t i = 0; i < 4; i++) draw_quadrant(i);
    }

    { /* Player Sprite */
        //use tile 32 as a "player":
        std::vector<glm::u8vec4> player_data;
        glm::uvec2 player_size;
        load_png(data_path("assets/bee-default.png"), &player_size, &player_data, LowerLeftOrigin);
        PPU466::Tile player_tile = generate_tile_from_data(player_data, ppu.palette_table[PlayerPalette]);
        ppu.tile_table[32] = player_tile;

        //player sprite:
        player_at = glm::vec2(32.0f, 216.0f); /* starting game position */
        ppu.sprites[32].index = 32;
        ppu.sprites[32].attributes = PlayerPalette;
    }


    { /* Other Sprites */
        //use sprite 12 as a "light":
        std::vector<glm::u8vec4> light_data;
        glm::uvec2 light_size;
        load_png(data_path("assets/light.png"), &light_size, &light_data, LowerLeftOrigin);
        PPU466::Tile light_tile = generate_tile_from_data(light_data, ppu.palette_table[LightPalette]);
        ppu.tile_table[12] = light_tile;

        /* Set the coordinates of the lights here (this part is hard-coded for now but there are only 4 values)
         * In the future, these should probably get read off another PNG file (along with flower sprites) */
        std::array< glm::uvec2, 4 > light_positions = {glm::uvec2(20, 20), glm::uvec2(164, 70),
                                                       glm::uvec2(76, 156), glm::uvec2(128, 164)};

        //4 light sprites in sprite table (take up sprite index 0-3, corresponding to their quadrant):
        for (size_t i = 0; i < 4; i++) {
            ppu.sprites[i].index = 12;
            ppu.sprites[i].attributes = LightPalette;
            ppu.sprites[i].x = light_positions[i].x;
            ppu.sprites[i].y = light_positions[i].y;
        }
    }
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
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
        } else if (evt.key.keysym.sym == SDLK_e) {
            E.downs += 1;
            E.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_q) {
            Q.downs += 1;
            Q.pressed = true;
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
        } else if (evt.key.keysym.sym == SDLK_e) {
            E.pressed = false;
            return true;
        } else if (evt.key.keysym.sym == SDLK_q) {
            Q.pressed = false;
            return true;
        }
    }

    return false;
}


void PlayMode::update(float elapsed) {
    constexpr float PlayerSpeed = 30.0f;
    constexpr float PlayerSize = 8.0f;
    constexpr float map_margin = 16.0f;
    constexpr float obj_margin = 8.0f;
    float distAttempted = PlayerSpeed * elapsed;
    float new_x = player_at.x;
    float new_y = player_at.y;

	// TODO: maybe make this a nicer looking function using Jim's advice from office hours, like the following:
//	{
//		float obj1_min_x = obj1_x - obj_margin;
//		float obj1_max_x = obj1_x + obj1_size + obj_margin;
//		float obj2_min_x = obj2_x - obj_margin;
//		float obj2_max_x = obj2_x + obj2_size + obj_margin;
//
//		float obj1_min_y = obj1_y - obj_margin;
//		float obj1_max_y = obj1_y + obj1_size + obj_margin;
//		float obj2_min_y = obj2_y - obj_margin;
//		float obj2_max_y = obj2_y + obj2_size + obj_margin;
//
//		if (obj1_max_x < obj2_min_x) return false;
//		if (obj2_max_x < obj1_min_x) return false;
//		if (obj1_max_y < obj2_min_y) return false;
//		if (obj2_max_y < obj2_min_y) return false;
//
//		return true;
//	}

    auto objects_overlap = []
            (float obj1_x, float obj1_y, float obj1_size, float obj2_x, float obj2_y, float obj2_size, float obj_margin){
        return ((obj1_x >= obj2_x - obj_margin && obj1_x <= obj2_x + obj2_size + obj_margin)
                || (obj1_x + obj1_size >= obj2_x - obj_margin && obj1_x + obj1_size <= obj2_x + obj2_size + obj_margin))
            && ((obj1_y >= obj2_y - obj_margin && obj1_y <= obj2_y + obj2_size + obj_margin)
                || (obj1_y + obj1_size >= obj2_y - obj_margin && obj1_y + obj1_size <= obj2_y + obj2_size + obj_margin));
    };

    if (left.pressed) new_x = std::max(player_at.x - distAttempted, map_margin);
    if (right.pressed) new_x = std::min(player_at.x + distAttempted, PPU466::ScreenWidth - map_margin - PlayerSize);
    if (down.pressed) new_y = std::max(player_at.y - distAttempted, map_margin + 1); // so bee doesn't touch the ground
    if (up.pressed) new_y = std::min(player_at.y + distAttempted, PPU466::ScreenHeight - map_margin - PlayerSize);

    auto player_overlapping_collider = [this, &objects_overlap, &new_x, &new_y](){
        /* Check collision with background maze tiles */
        for (size_t i = 0; i < ppu.BackgroundHeight; i++) {
            for (size_t j = 0; j < ppu.BackgroundWidth; j++) {
                size_t current_pixel_x = j * 8;
                size_t current_pixel_y = i * 8;
                size_t current_tile_idx = (i * ppu.BackgroundWidth) + j;
                if (((ppu.background[current_tile_idx] & 0b11111111) > 0 && (ppu.background[current_tile_idx] & 0b11111111) < 10)
                  && objects_overlap(new_x, new_y, PlayerSize, (float)current_pixel_x, (float)current_pixel_y, 8, 1.0f)) {
                    std::cout << "Hitting the collider at pixel (" << current_pixel_x << ", " << current_pixel_y << ")\n";
                    std::cout << "current pixel index is" << current_tile_idx;
                    return true;
                }
            }
        }
        return false;
    };

    if (!player_overlapping_collider()) {
        player_at.x = new_x;
        player_at.y = new_y;
    }

    /* handle interactions with sprite objects */
    if (E.pressed) {
        for (size_t i = 0; i < 4; i++) { /* check all the light objects */
            uint8_t current_light_x = ppu.sprites[i].x;
            uint8_t current_light_y = ppu.sprites[i].y;
            if (objects_overlap(player_at.x, player_at.y, PlayerSize, current_light_x, current_light_y, 8, obj_margin)) {
                illuminate_quadrant(i);

                //TODO: make the corresponding flower objects visible
            }
        }
    }

    /* quit game */
    if (Q.pressed) PlayMode::set_current(nullptr);

    //reset button press counters:
    left.downs = 0;
    right.downs = 0;
    up.downs = 0;
    down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
    //--- set ppu state based on game state ---

    //player sprite:
    ppu.sprites[32].x = int8_t(player_at.x);
    ppu.sprites[32].y = int8_t(player_at.y);

    //--- actually draw ---
    ppu.draw(drawable_size);
}
