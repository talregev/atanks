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

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
sky.cc

Code for generating sky backgrounds, including moons.
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

/*
TODO
 + Improve documentation
 + Add clouds?
*/

#include "externs.h"
#include "main.h"
#include <vector>
#include "environment.h"
#include "sky.h"
#include "files.h"
#include "gfxData.h"
#include "gameloop.h"


/*****************************************************************************
Static temp sky bitmap for faster sky creation
*****************************************************************************/
static BITMAP* temp_sky = nullptr;


/*****************************************************************************
Static helper function prototypes
*****************************************************************************/
static double  central_rand (double u);
static int32_t clamped_int  (int32_t m, int32_t a, int32_t z);
static double  coverage     (double distance, double radius);
static void    draw_moons   (LevelCreator* lcr, int32_t width, int32_t height);


/*============================================================================
struct moon

A simple data structure to store the parameters of a moon for easy passing.
============================================================================*/
struct moon
{
	BITMAP* bitmap;
	int32_t   col1;
	int32_t   col2;
	double          lambda;
	int32_t         octaves;
	int32_t         radius;
	double          smoothness;
	int32_t         x;
	double          xoffset;
	int32_t         y;
	double          yoffset;

	// Simple ctor:
	explicit moon(int32_t scrnw, int32_t scrnh) :
		col1      (makecol(rand() % 255, rand() % 255, rand() % 255)),
		col2      (makecol(rand() % 255, rand() % 255, rand() % 255)),
		lambda    (((rand() % 60) + 30) / 100.),
		octaves   ((rand() % 4) + 6),
		radius    (static_cast<int32_t>(central_rand(scrnw / 8) + .5)),
		smoothness((rand() % 20) + 3),
		x         (rand() % scrnw),
		xoffset   (rand()),
		y         (rand() % scrnh),
		yoffset   (rand())
	{
		bitmap = create_bitmap (radius * 2, radius * 2);
	}

	// Simple dtor to get rid of the temp bitmap
	~moon()
	{
		if (bitmap)
			destroy_bitmap(bitmap);
	}
};



/*############################################################################
ZBuffer

Acts a a simple, 1bpp zbuffer.  For each pixel location, the ZBuffer can
remember if something is "popping up" at that location.
############################################################################*/
class ZBuffer
{
public:
	// No copies:
	ZBuffer() = delete;
	ZBuffer& operator=(const ZBuffer&) = delete;

    /*************************************************************************
    ctor

    Construct a ZBuffer object capable of storing "popup" values for a
    w by h grid.  All cells in the ZBuffer start out lowered.
    *************************************************************************/
	ZBuffer( int32_t w, int32_t h )
	{
		int32_t width = w;
		while ( width ) {
			width >>= 1;
			++shiftamt;
		}
		z.resize( (h << shiftamt) | w );
	}


    /*************************************************************************
    test

    Returns true if the cell at location (x,y) is raised.  Behaviour is
    undefined if x does not fall in the range [0,w) or if y does not fall in
    the range [0,h); w and h being the parameters to the ctor.
    *************************************************************************/
	bool test( int32_t x, int32_t y ) const
	{
		try {
			return z.at((y << shiftamt) | x);
		} catch (...) {
			return false;
		}
	}


    /*************************************************************************
    set

    Causes a cell in the ZBuffer to become raised.  Follows the same
    conditions on x and y as the test function does.
    *************************************************************************/
	void set( int32_t x, int32_t y )
	{
		try {
			z.at((y<<shiftamt) | x) = true ;
		} catch (...) { /* nothing can be done here... */ }
	}

private:
	std::vector< bool >	z;
	int32_t             shiftamt = 0;
};


/*****************************************************************************
Static function prototypes that need either moon or ZBuffer
*****************************************************************************/
static void    draw_amoon   (LevelCreator* lcr, const moon &mn,
                             int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                             bool darkside, ZBuffer &zbuffer);
static void    paint_moonpix(int32_t x, int32_t y,
                             const moon &mn, double xval, double yval,
                             double blend);



/*****************************************************************************
central_rand

Return a random double in the range [0,u] where non-extreme values
are preferred.

Basic on a simple cubic function.
*****************************************************************************/
static double central_rand(double u)
{
	const double x = static_cast<double>(rand()) / static_cast<double>(RAND_MAX)
	               - 0.5; // [-.5,+.5]
	return u * (0.5 - (x*x*x)*4.0) ;
}


/*****************************************************************************
clamped_int

Clamps an integer value, m, into a range specified by [a,z].  Returns the
clamped value.
*****************************************************************************/
static int32_t clamped_int(int32_t m, int32_t a, int32_t z)
{
	return ( m < a ? a : ( m > z ? z : m ) );
}


/*****************************************************************************
coverage

Compute the percent coverage of a pixel by a sphere given the pixel's
distance from the centre and the sphere's radius.
*****************************************************************************/
static double coverage(double distance, double radius)
{
	if ( distance > radius )
		return 1 - (distance - radius);
	return 1. ;
}


/*****************************************************************************
draw_amoon

Renders a single moon onto a bitmap.  Assumes transparent drawing is enabled.
Obeys the given bounding box, which may be smaller than the moon itself.
Uses the darkside parameter to decide which side of the moon should be
dark.  Obeys and updates the z-buffer.

The current implementation of this function is begging for some
simplifications.  And again, what about those [xy]offset variables?
*****************************************************************************/
static void draw_amoon(LevelCreator* lcr, const moon& mn,
                       int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                       bool darkside, ZBuffer& zbuffer)
{
	int32_t startX = std::min(x0, x1);
	int32_t endX   = std::max(x0, x1);
	int32_t startY = std::min(y0, y1);
	int32_t endY   = std::max(y0, y1);

	clear_to_color(mn.bitmap, BLACK);
	blit(temp_sky, mn.bitmap, startX, startY, 0, 0, mn.radius * 2, mn.radius * 2);

	for (int32_t y = startY; (y < endY) && lcr->can_work(); ++y ) {
		bool hityet = false;

		for (int32_t x = startX; (x < endX) && lcr->can_work(); ++x ) {
			/* Occupied? */
			if ( zbuffer.test(x,y) )
				continue ;

			/* Find distance from this moon */
			int32_t xdist = mn.x - x;
			int32_t ydist = mn.y - y;

			/* Compute some other nice circle values */
			const
			double radius    = mn.radius ;
			double xval      = static_cast<double>(xdist) / radius;
			double yval      = static_cast<double>(ydist) / radius;
			double distance2 = (xdist * xdist) + (ydist * ydist);
			double distance  = std::sqrt(distance2);

			/* A bound check -> are we in the circle? */
			if ( distance > (radius + 1) ) {
				if (hityet) // If we've already been inside at this y...
					break;  // then skip ahead to the next y
				continue ;  // Otherwise stay at this y, and skip to the next x
			}

			/* Edges use lighter blending */
			const double edgeval = coverage(distance, radius);

			/* Now, should we paint this side of the moon? */
			if (xval && ( (xval < 0) == darkside) ) {
				lcr->yield();
				paint_moonpix(x - startX, y - startY, mn, fabs(xval), yval, edgeval);
			}

			/* Mark this pixel as occupied */
			zbuffer.set(x,y);
			hityet = true ;
		}
	}

	// Put the moon on the sky bitmap:
	global.lockLand();
	drawing_mode (DRAW_MODE_TRANS, NULL, 0, 0);
	blit (mn.bitmap, temp_sky, 0, 0,
		  startX, startY, mn.radius * 2, mn.radius * 2);
	drawing_mode(global.current_drawing_mode, NULL, 0, 0);
	global.unlockLand();
}



/*****************************************************************************
paint_moonpix

Paint a pixel onto the screen for a particular part of a moon.

Parameters:
* bmp, x, y
	The bitmap on which to paint.  Paints onto the pixel at (x,y)
* mn, xval, yval
	The moon to paint.  The val's give the percentage along the moon.
	Percentages must fall within [0,1].
* blend
	Controls how much "paint" is used.  Must be in the range [0,1].  Higher
	values cause stronger painting.  Used for anti-aliasing.

*****************************************************************************/
static void paint_moonpix(int32_t x, int32_t y,
                          const moon &mn, double xval, double yval,
                          double blend)
{
	const double thetax  = RAD2DEG(asin(xval));
	const double thetay  = RAD2DEG(acos(yval));
	const double offset  = (perlin2DPoint (1., mn.smoothness,
	                                       mn.xoffset + mn.x + thetax,
	                                       mn.yoffset + mn.y + thetay,
	                                       mn.lambda, mn.octaves) + 1.) / 2.;
	const double percVal = (perlin2DPoint (1.0, mn.smoothness,
	                                       mn.xoffset + mn.x * 1000 + thetax,
	                                       mn.yoffset + mn.y * 1000 + thetay,
	                                       mn.lambda, mn.octaves) + 1) / 2;

	set_add_blender (0, 0, 0, blend * xval * percVal * offset * 255);
	drawing_mode (DRAW_MODE_TRANS, NULL, 0, 0);
	putpixel (mn.bitmap, x, y, mn.col1);
	set_add_blender (0, 0, 0, blend * xval * (1. - percVal) * offset * 255);
	putpixel (mn.bitmap, x, y, mn.col2);
	drawing_mode (global.current_drawing_mode, NULL, 0, 0);
}


/*****************************************************************************
draw_moons

Renders a set of moons over a given bitmap.  The bitmap to draw of and the
appropriate dimensions must be given.
*****************************************************************************/
static void draw_moons (LevelCreator* lcr, int32_t width, int32_t height)
{
	const bool darkside = rand() > (RAND_MAX / 2 + 1);
	ZBuffer	   zbuffer( width, height ) ;

	for (int32_t numMoons = central_rand( 14.0 ); numMoons; --numMoons) {
		/* Make up a moon */
		const moon mn(width, height);

		/* Where is it? */
		int32_t x0 = clamped_int(mn.x - mn.radius, 0, width );
		int32_t y0 = clamped_int(mn.y - mn.radius, 0, height);
		int32_t x1 = clamped_int(mn.x + mn.radius, 0, width );
		int32_t y1 = clamped_int(mn.y + mn.radius, 0, height);

		/* Draw it */
		draw_amoon(lcr, mn, x0, y0, x1, y1, darkside, zbuffer);
	}
}



/*****************************************************************************
generate_sky

Given some input parameters, renders a sky (with moons) onto a bitmap.
*****************************************************************************/
void generate_sky(LevelCreator* lcr, const gradient* grad, int32_t flags )
{
	double messiness  = (static_cast<double>(rand () % 100) / 1000.0 + 0.05);
	const int xoffset = rand() % env.screenWidth;  // For perlin, random starting x
	const int yoffset = rand() % env.screenHeight; // For perlin, random starting y

	temp_sky = create_bitmap( env.sky->w, env.sky->h );
	clear_to_color(temp_sky, BLACK);
	clear_to_color(env.sky, BLACK);

	for (int32_t x = 0
	     ; (!lcr || lcr->can_work()) && (x < env.screenWidth)
	     ; ++x) {
		for (int32_t y = 0
		     ; (!lcr || lcr->can_work()) && (y < (env.screenHeight - MENUHEIGHT))
		     ; ++y) {

			lcr->yield();

			double offset = 0;

			if ( flags & GENSKY_DETAILED )
				offset += perlin2DPoint(1., 200, xoffset + x, yoffset + y, .3, 6)
				        * (static_cast<double>(env.screenHeight - MENUHEIGHT)
				           * messiness);

			if ( flags & GENSKY_DITHERGRAD )
				offset += (rand () % 10) - 5;

			while ( ( (y + offset) < 0)
			     || ( (y + offset + 1) > (env.screenHeight - MENUHEIGHT) ) )
				offset /= 2;

			global.lockLand();
			solid_mode();
			putpixel (temp_sky, x, y,
			          gradientColorPoint(grad, env.screenHeight - MENUHEIGHT,
			                             y + offset));
			drawing_mode(global.current_drawing_mode, NULL, 0, 0);
			global.unlockLand();
		}
	}
	draw_moons (lcr, env.screenWidth, env.screenHeight - MENUHEIGHT);

	// Put temp sky onto the real bitmap:
	global.lockLand();
	solid_mode();
	blit(temp_sky, env.sky, 0, 0, 0, 0,
	     env.sky->w, env.sky->h);
	global.unlockLand();

	// clean up
	if (temp_sky)
		destroy_bitmap(temp_sky);
	temp_sky = nullptr;
}

