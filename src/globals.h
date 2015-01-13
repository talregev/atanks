#ifndef GLOBALS_DEFINE
#define GLOBALS_DEFINE

/*
 * atanks - obliterate each other with oversize weapons
 * Copyright (C) 2003  Thomas Hudson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * */

/*
#include "virtobj.h"
#include "floattext.h"
#include "physobj.h"
#include "tank.h"
#include "missile.h"
#include "explosion.h"
#include "player.h"
#include "environment.h"
*/
#include "globaldata.h"
/*
#include "teleport.h"
#include "decor.h"
#include "beam.h"
*/
using namespace std;

char *errorMessage;
int errorX, errorY;

int WHITE, BLACK, PINK, RED, GREEN, DARK_GREEN, BLUE, PURPLE, YELLOW;

int k;
int ctrlUsedUp;

const gradient topbar_gradient[] =
{
    {{200,200,200,0}, 0.0},
    {{255,255,255,0}, 0.5},
    {{128,128,128,0}, 1.0},
    {{0,0,0,0}, -1}
};

const gradient stuff_bar_gradient[] =
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

const gradient circles_gradient[] =
{
    {{100, 75, 50,0}, 0.0},
    {{  0,100,  0,0}, 0.5},
    {{255,255,255,0}, 1.0},
    {{0,0,0,0}, -1}
};

// Explosion gradient
const gradient explosion_gradient1[] =
{
    {{150, 75, 30,0}, 0.0},
    {{255,255,255,0}, 1.0},
    {{0,0,0,0}, -1}
};

// Explosion gradient
const gradient explosion_gradient2[] =
{
    {{255,255,102,0}, 0.0}, // ff6
    {{255,136, 0,0}, 1.0}, // f80
    {{0,0,0,0}, -1}
};

const gradient * const explosion_gradients[] =
{
    explosion_gradient1,
    explosion_gradient2
};

/*
// Sky gradients, first line is top of screen
const gradient sky_gradient1[] =
{
    {{255,255,255,0}, 0.0},
    {{100,100,100,0}, 0.1},
    {{ 80,100,100,0}, 0.5},
    {{0,0,0,0}, -1}
};

const gradient sky_gradient2[] =
{
    {{  0,  0, 40,0}, 0.0},
    {{ 40, 40,100,0}, 0.5},
    {{ 80,80,100,0}, 0.75},
    {{0,0,0,0}, -1}
};

const gradient sky_gradient3[] =
{
    {{240,  0, 40,0}, 0.0},
    {{140, 40,100,0}, 0.15},
    {{ 80, 80,100,0}, 0.75},
    {{0,0,0,0}, -1}
};

const gradient sky_gradient4[] =
{
    {{ 40, 40, 40,0}, 0.0},
    {{100, 40,100,0}, 0.2},
    {{ 80, 80,100,0}, 0.75},
    {{0,0,0,0}, -1}
};

const gradient sky_gradient5[] =
{
    {{  0, 90, 40,0}, 0.0},
    {{  0,120,100,0}, 0.2},
    {{ 40,200,100,0}, 0.75},
    {{0,0,0,0}, -1}
};

// Sunset
const gradient sky_gradient6[] =
{
    {{ 70,240,240,0}, 0.0},
    {{ 70,200,200,0}, 0.3},
    {{ 70,200,160,0}, 0.35},
    {{255,200, 70,0}, 0.6},
    {{255,255,128,0}, 1.0},
    {{0,0,0,0}, -1}
};

// Burning sky
const gradient sky_gradient7[] =
{
    {{ 20, 20, 20,0}, 0.0},
    {{255,200,  0,0}, 0.08},
    {{255,255,  0,0}, 0.13},
    {{ 20, 20, 20,0}, 0.16},
    {{255,200,  0,0}, 0.5},
    {{255,255,  0,0}, 1.0},
    {{0,0,0,0}, -1}
};

// Burning landscape, black skies
const gradient sky_gradient8[] =
{
    {{  0,  0,  0,0}, 0.0},
    {{100,  0,  0,0}, 0.4},
    {{255,255,255,0}, 0.8},
    {{0,0,0,0}, -1}
};

// Sky gradients, first line is top of screen
// dark blue to darker blue
const gradient sky_gradient9[] =
{
    {{ 90, 90,255,0}, 0.0},
    {{ 60, 60,200,0}, 0.3},
    {{ 30, 30,150,0}, 1.0},
    {{0,0,0,0}, -1}
};

// dark blue to blue-grey
const gradient sky_gradient10[] =
{
    {{110,110,255,0}, 0.0},
    {{150,150,255,0}, 0.3},
    {{200,200,255,0}, 1.0},
    {{0,0,0,0}, -1}
};

// white to grey-blue to dark blue
const gradient sky_gradient11[] =
{
    {{255,255,255,0}, 0.0},
    {{200,200,255,0}, 0.3},
    {{ 80, 80,180,0}, 1.0},
    {{0,0,0,0}, -1}
};

// simple purple: dark to light
const gradient sky_gradient12[] =
{
    {{133, 33,133,0}, 0.0},
    {{220,120,220,0}, 1.0},
    {{0,0,0,0}, -1}
};

// night sky: black to dark purple
const gradient sky_gradient13[] =
{
    {{  0,  0,  0,0}, 0.0},
    {{ 50,  0, 50,0}, 1.0},
    {{0,0,0,0}, -1}
};

// Sunset
const gradient sky_gradient14[] =
{
    {{  0,  0,  0,0}, 0.0},
    {{ 50,  0, 50,0}, 0.1},
    {{ 90, 20,  0,0}, 0.3},
    {{180, 50,  0,0}, 0.7},
    {{255,150,150,0}, 0.9},
    {{255,255,100,0}, 1.0},
    {{0,0,0,0}, -1}
};

// Burning sky
const gradient sky_gradient15[] =
{
    {{185, 60, 60,0}, 0.0},
    {{240,110,110,0}, 0.5},
    {{255,110,110,0}, 1.0},
    {{0,0,0,0}, -1}
};

// Burning landscape, black skies
const gradient sky_gradient16[] =
{
    {{  0,  0,  0,0}, 0.0},
    {{170, 50, 50,0}, 0.5},
    {{220,110,110,0}, 1.0},
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
*/

int cclock, fps, frames, fi, lx, ly;
#define CLOCK_MAX 10

int steep=2, mheight=200, mbase=0;
double msteep=.2, smooth=.00;

int winner;

char buf[100];

bool quit_right_now;

WEAPON weapon[WEAPONS];
WEAPON naturals[NATURALS];
ITEM item[ITEMS];

GLOBALDATA *my_global;

#endif
