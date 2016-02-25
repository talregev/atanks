#ifndef ATANKS_SRC_ATANKS_CPP
#	error "globals.h must not be included from anywhere but atanks.cpp!"
#endif // ATANKS_SRC_ATANKS_CPP

#include "globaldata.h"

// === The two most important things in the game: ;) ===
GLOBALDATA  global;
ENVIRONMENT env;

// === Defined colours used everywhere ===
int32_t BLACK, BLUE, DARK_GREEN, DARK_GREY, DARK_RED, GOLD, GREY,
        GREEN, LIGHT_GREEN, LIME_GREEN, ORANGE, PINK, PURPLE, RED,
        SILVER, TURQUOISE, WHITE, YELLOW;

// === General values that are globally used ===
char        buf[100]; // buffer for general use
const char* errorMessage;
int32_t     errorX, errorY;
int32_t	    k, K; // k = key pressed, K = Key Code from k
int32_t     fi, lx, ly;
int32_t     game_version; // Used for update check and save file upgrades


// === Gradients ===
gradient topbar_gradient[4] =
{
	{{200,200,200,0}, 0.0},
	{{255,255,255,0}, 0.5},
	{{128,128,128,0}, 1.0},
	{{0,0,0,0}, -1}
};

gradient stuff_bar_gradient[11] =
{
	{{  0,120,  0,0}, 0.0},
	{{ 10,210, 50,0}, 0.1},
	{{100,150,150,0}, 0.28},
	{{100,170,170,0}, 0.31},
	{{200,200,200,0}, 0.33},
	{{120,150,120,0}, 0.35},
	{{180,190,180,0}, 0.5},
	{{210,210,210,0}, 0.55},
	{{200,220,200,0}, 0.57},
	{{255,255,255,0}, 1.0},
	{{0,0,0,0}, -1}
};

gradient circles_gradient[4] =
{
	{{100, 75, 50,0}, 0.0},
	{{  0,100,  0,0}, 0.5},
	{{255,255,255,0}, 1.0},
	{{0,0,0,0}, -1}
};

// Explosion gradient
gradient explosion_gradient1[3] =
{
	{{150, 75, 30,0}, 0.0},
	{{255,255,255,0}, 1.0},
	{{0,0,0,0}, -1}
};

// Explosion gradient
gradient explosion_gradient2[3] =
{
	{{255,255,102,0}, 0.0}, // ff6
	{{255,136, 0,0}, 1.0}, // f80
	{{0,0,0,0}, -1}
};

gradient* explosion_gradients[2] =
{
  explosion_gradient1,
  explosion_gradient2
};
