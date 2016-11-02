#ifndef LAND_HEADER_FILE__
#define LAND_HEADER_FILE__

#include "main.h"

#define LANDS 8
// The first LANDS are regular, the second are crispy.

const gradient land_gradient1[] =
{
	{{ 20, 40, 20,0}, 0.0f},
	{{ 80,100, 80,0}, 0.5f},
	{{ 80,120,100,0}, 1.0f},
	{{0,0,0,0}, -1}
};

const gradient land_gradient2[] =
{
	{{100,200,100,0}, 0.0f},
	{{ 80,100, 80,0}, 0.5f},
	{{ 80,120,100,0}, 1.0f},
	{{0,0,0,0}, -1}
};

const gradient land_gradient3[] =
{
	{{200,100,100,0}, 0.0f},
	{{100, 70, 80,0}, 0.5f},
	{{120, 80,100,0}, 1.0f},
	{{0,0,0,0}, -1}
};

const gradient land_gradient4[] =
{
	{{ 80, 50, 60,0}, 0.0f},
	{{100, 70, 80,0}, 0.25f},
	{{150,120, 80,0}, 0.75f},
	{{200,180,150,0}, 1.0f},
	{{0,0,0,0}, -1}
};

const gradient land_gradient5[] =
{
	{{ 20, 20, 20,0}, 0.0f},
	{{100,100,100,0}, 0.7f},
	{{250,250,250,0}, 0.75f},
	{{250,250,250,0}, 1.0f},
	{{0,0,0,0}, -1}
};

const gradient land_gradient6[] =
{
	{{200,180, 70,0}, 0.0f},
	{{100, 90, 30,0}, 1.0f},
	{{0,0,0,0}, -1}
};

const gradient land_gradient7[] =
{
	{{ 50,200,150,0}, 0.0f},
	{{130,120,180,0}, 1.0f},
	{{0,0,0,0}, -1}
};

const gradient land_gradient8[] =
{
	{{  0,  0,  0,0}, 0.0f},
	{{ 50, 40, 50,0}, 0.4f},
	{{100,  0,  0,0}, 0.8f},
	{{0,0,0,0}, -1}
};

const gradient land_gradient9[] =
{
	{{100,100,100,0}, 0.0f},
	{{ 50, 50, 50,0}, 0.7f},
	{{255,255,255,0}, 1.0f},
	{{0,0,0,0}, -1}
};

const gradient land_gradient10[] =
{
	{{100, 50, 50,0}, 0.0f},
	{{ 50,  0,  0,0}, 0.25f},
	{{100, 10, 10,0}, 0.5f},
	{{100,  0,  0,0}, 0.95f},
	{{  0,  0,  0,0}, 1.0f},
	{{0,0,0,0}, -1}
};

const gradient land_gradient11[] =
{
	{{ 50,100, 50,0}, 0.0f},
	{{  0, 50,  0,0}, 0.7f},
	{{  0,100,  0,0}, 0.95f},
	{{  0,  0,  0,0}, 1.0f},
	{{0,0,0,0}, -1}
};

const gradient land_gradient12[] =
{
	{{ 90, 90, 90,0}, 0.0f},
	{{ 30, 30, 30,0}, 0.4f},
	{{ 60, 60, 60,0}, 0.95f},
	{{  0,  0,  0,0}, 1.0f},
	{{0,0,0,0}, -1}
};

const gradient land_gradient13[] =
{
	{{ 90, 90, 90,0}, 0.0f},
	{{  0, 60,  0,0}, 0.95f},
	{{  0,  0,  0,0}, 1.0f},
	{{0,0,0,0}, -1}
};

const gradient land_gradient14[] =
{
	{{  0,175,  0,0}, 0.0f},
	{{  0, 50,  0,0}, 0.95f},
	{{  0,  0,  0,0}, 1.0f},
	{{0,0,0,0}, -1}
};

const gradient land_gradient15[] =
{
	{{100, 50, 50,0}, 0.0f},
	{{ 50,  0,  0,0}, 0.95f},
	{{  0,  0,  0,0}, 1.0f},
	{{0,0,0,0}, -1}
};

const gradient land_gradient16[] =
{
	{{200,  0,  0,0}, 0.0f},
	{{200,200,  0,0}, 0.25f},
	{{  0,200,  0,0}, 0.5f},
	{{  0,  0,200,0}, 0.75f},
	{{200,  0,200,0}, 1.0f},
	{{0,0,0,0}, -1}
};

const gradient * const land_gradients[] =
{
	land_gradient1,
	land_gradient2,
	land_gradient3,
	land_gradient4,
	land_gradient5,
	land_gradient6,
	land_gradient7,
	land_gradient8,
	land_gradient9,
	land_gradient10,
	land_gradient11,
	land_gradient12,
	land_gradient13,
	land_gradient14,
	land_gradient15,
	land_gradient16
};

class LevelCreator;

void generate_land(LevelCreator* lcr, int32_t yoffset, int32_t heightx);

#endif

