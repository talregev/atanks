#ifndef SKY_HEADER_FILE__
#define SKY_HEADER_FILE__

#include "main.h"

#define SKIES 8
// The first SKIES are regular, the second are crispy.

// Sky gradients, first line is top of screen
const gradient sky_gradient1[] =
{
	{{255,255,255,0}, 0.0f},
	{{100,100,100,0}, 0.1f},
	{{ 80,100,100,0}, 0.5f},
	{{0,0,0,0}, -1}
};

const gradient sky_gradient2[] =
{
	{{  0,  0, 40,0}, 0.0f},
	{{ 40, 40,100,0}, 0.5f},
	{{ 80,80,100,0}, 0.75f},
	{{0,0,0,0}, -1}
};

const gradient sky_gradient3[] =
{
	{{240,  0, 40,0}, 0.0f},
	{{140, 40,100,0}, 0.15f},
	{{ 80, 80,100,0}, 0.75f},
	{{0,0,0,0}, -1}
};

const gradient sky_gradient4[] =
{
	{{ 40, 40, 40,0}, 0.0f},
	{{100, 40,100,0}, 0.2f},
	{{ 80, 80,100,0}, 0.75f},
	{{0,0,0,0}, -1}
};

const gradient sky_gradient5[] =
{
	{{  0, 90, 40,0}, 0.0f},
	{{  0,120,100,0}, 0.2f},
	{{ 40,200,100,0}, 0.75f},
	{{0,0,0,0}, -1}
};

// Sunset
const gradient sky_gradient6[] =
{
	{{ 70,240,240,0}, 0.0f},
	{{ 70,200,200,0}, 0.3f},
	{{ 70,200,160,0}, 0.35f},
	{{255,200, 70,0}, 0.6f},
	{{255,255,128,0}, 1.0f},
	{{0,0,0,0}, -1}
};

// Burning sky
const gradient sky_gradient7[] =
{
	{{ 20, 20, 20,0}, 0.0f},
	{{255,200,  0,0}, 0.08f},
	{{255,255,  0,0}, 0.13f},
	{{ 20, 20, 20,0}, 0.16f},
	{{255,200,  0,0}, 0.5f},
	{{255,255,  0,0}, 1.0f},
	{{0,0,0,0}, -1}
};

// Burning landscape, black skies
const gradient sky_gradient8[] =
{
	{{  0,  0,  0,0}, 0.0f},
	{{100,  0,  0,0}, 0.4f},
	{{255,255,255,0}, 0.8f},
	{{0,0,0,0}, -1}
};

// Sky gradients, first line is top of screen
// dark blue to darker blue
const gradient sky_gradient9[] =
{
	{{ 90, 90,255,0}, 0.0f},
	{{ 60, 60,200,0}, 0.3f},
	{{ 30, 30,150,0}, 1.0f},
	{{0,0,0,0}, -1}
};

// dark blue to blue-grey
const gradient sky_gradient10[] =
{
	{{110,110,255,0}, 0.0f},
	{{150,150,255,0}, 0.3f},
	{{200,200,255,0}, 1.0f},
	{{0,0,0,0}, -1}
};

// white to grey-blue to dark blue
const gradient sky_gradient11[] =
{
	{{255,255,255,0}, 0.0f},
	{{200,200,255,0}, 0.3f},
	{{ 80, 80,180,0}, 1.0f},
	{{0,0,0,0}, -1}
};

// simple purple: dark to light
const gradient sky_gradient12[] =
{
	{{133, 33,133,0}, 0.0f},
	{{220,120,220,0}, 1.0f},
	{{0,0,0,0}, -1}
};

// night sky: black to dark purple
const gradient sky_gradient13[] =
{
	{{  0,  0,  0,0}, 0.0f},
	{{ 50,  0, 50,0}, 1.0f},
	{{0,0,0,0}, -1}
};

// Sunset
const gradient sky_gradient14[] =
{
	{{  0,  0,  0,0}, 0.0f},
	{{ 50,  0, 50,0}, 0.1f},
	{{ 90, 20,  0,0}, 0.3f},
	{{180, 50,  0,0}, 0.7f},
	{{255,150,150,0}, 0.9f},
	{{255,255,100,0}, 1.0f},
	{{0,0,0,0}, -1}
};

// Burning sky
const gradient sky_gradient15[] =
{
	{{185, 60, 60,0}, 0.0f},
	{{240,110,110,0}, 0.5f},
	{{255,110,110,0}, 1.0f},
	{{0,0,0,0}, -1}
};

// Burning landscape, black skies
const gradient sky_gradient16[] =
{
	{{  0,  0,  0,0}, 0.0f},
	{{170, 50, 50,0}, 0.5f},
	{{220,110,110,0}, 1.0f},
	{{0,0,0,0}, -1}
};


const gradient * const sky_gradients[] =
{
	sky_gradient1,
	sky_gradient2,
	sky_gradient3,
	sky_gradient4,
	sky_gradient5,
	sky_gradient6,
	sky_gradient7,
	sky_gradient8,
	sky_gradient9,
	sky_gradient10,
	sky_gradient11,
	sky_gradient12,
	sky_gradient13,
	sky_gradient14,
	sky_gradient15,
	sky_gradient16
};


#define GENSKY_DETAILED		1
#define GENSKY_DITHERGRAD	2

class LevelCreator;

void generate_sky(LevelCreator* lcr, const gradient* grad, int32_t flags);

#endif
