#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

    /* used to index into the palette table with more human-readable values */
    enum PaletteTableIndex : size_t {
        GroundPalette = 0,
        MazeLitPalette = 1,
        MazeUnlitPalette = 2,
        PlayerPalette = 3,
        LightPalette = 4,
    };

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, E, Q;

	//player position:
	glm::vec2 player_at = glm::vec2(0.0f);

    // background quadrant info and function to update palette:
    std::array< std::string, 4 > magic_values = {"Q_LL", "Q_LR", "Q_UL", "Q_UR"};
    std::array< std::vector< char >, 4 > quadrant_chunks;
    std::size_t quadrant_height = PPU466::BackgroundHeight / 4;
    std::size_t quadrant_width = PPU466::BackgroundWidth / 4;
    std::array< size_t, 4 > start_idxs;

    void draw_quadrant(uint8_t quadrant);
    void illuminate_quadrant(uint8_t quadrant);

	//----- drawing handled by PPU466 -----
	PPU466 ppu;
};
