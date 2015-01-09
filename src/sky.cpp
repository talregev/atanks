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

#include "globaldata.h"
#include "main.h"
#include <vector>
#include "environment.h"
#include "sky.h"
#include "files.h"


/* Here's a function that could move nicely out of atanks.cc :) */
int gradientColorPoint (const gradient *grad, double length, double line);


/*============================================================================
struct moon

A simple data structure to store the parameters of a moon for easy passing.
============================================================================*/
struct moon
  {
    int radius;
    int x;
    int y;
    double lambda;
    int octaves;
    double smoothness;
    double xoffset;
    double yoffset;
    int col1;
    int col2;
  };


/*############################################################################
ZBuffer

Acts a a simple, 1bpp zbuffer.  For each pixel location, the ZBuffer can
remember if something is (char *)"popping up" at that location.
############################################################################*/
class ZBuffer
  {
  private:
    // empty ctor, copy-ctor and assign operator are private, so the compiler won't create implicit ones!
    inline ZBuffer () { }
    inline ZBuffer (ZBuffer &sourceZBuf _UNUSED)_UNUSED;
    inline const ZBuffer& operator= (const ZBuffer &sourceZBuf) { return(sourceZBuf); }

    std::vector< bool >	z;
    int shiftamt;

  public:
    /*************************************************************************
    ctor

    Construct a ZBuffer object capable of storing (char *)"popup" values for a
    w by h grid.  All cells in the ZBuffer start out lowered.
    *************************************************************************/
    ZBuffer( int w, int h )
    {
      shiftamt = 0;
      while ( w )
        {
          w >>= 1;
          ++shiftamt;
        }
      z.resize( h << shiftamt );
    }

    /*************************************************************************
    test

    Returns true iff the cell at location (x,y) is raised.  Behavior is
    undefined if x does not fall in the range [0,w) or if y does not fall in
    the range [0,h); w and h being the parameters to the ctor.
    *************************************************************************/
    bool test( int x, int y ) const
      {
        return z[ (y<<shiftamt) | x ];
      }


    /*************************************************************************
    set

    Causes a cell in the ZBuffer to become raised.  Follows the same
    conditions on x and y as the test function does.
    *************************************************************************/
    void set( int x, int y )
    {
      z[ (y<<shiftamt) | x ] = true ;
    }
  };


/*****************************************************************************
central_rand

Return a random double in the range [0,u] where non-extreme values
are preferred.

Basic on a simple cubic function.
*****************************************************************************/
static double central_rand( double u )
{
  const double x = ( (double)rand( ) / RAND_MAX ) - 0.5;	// [-.5,+.5]
  return u * (0.5 - (x*x*x)*4.0) ;
}


/*****************************************************************************
clamped_int

Clamps an integer value, m, into a range specified by [a,z].  Returns the
clamped value.
*****************************************************************************/
static int clamped_int( int m, int a, int z )
{
  return (m<a) ? a : ( (m>z) ? z : m );
}


/*****************************************************************************
generate_moon

Returns a moon structure with appropriately randomized variables.
*****************************************************************************/
static moon generate_moon( int scrnw, int scrnh )
{
  moon m;

  m.smoothness = (rand () % 20) + 3;
  m.octaves = rand () % 4 + 6;
  m.lambda = (rand () % 60 + 30) * (1.00/100);

  //m.radius = (int)((rand () % 100 / 200.0) * (rand () % 100 / 200.0) * scrnw);
  m.radius = (int)central_rand( scrnw/8 ) ;
  m.x = rand () % scrnw;
  m.y = rand () % scrnh;

  m.xoffset = rand ();
  m.yoffset = rand ();

  m.col1 = makecol (rand () % 255, rand () % 255, rand () % 255);
  m.col2 = makecol (rand () % 255, rand () % 255, rand () % 255);

  return m;
}


/*****************************************************************************
coverage

Compute the percent coverage of a pixel by a sphere given the pixel's
distance from the center and the sphere's radius.
*****************************************************************************/
static double coverage( double distance, double fradius )
{
  if ( distance <= fradius )
    return 1.0 ;
  return 1 - (distance - fradius);
}


/*****************************************************************************
fract_clamp - unused

Clamp a fraction down to the range [0.0,1.0]
*****************************************************************************/
/*static double fract_clamp( double x ) {
	return (x < 0.0) ? 0.0 : ( (x>1.0) ? 1.0 : x );
}*/



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
	Controls how much (char *)"paint" is used.  Must be in the range [0,1].  Higher
	values cause stronger painting.  Used for anti-aliasing.

*****************************************************************************/
static void paint_moonpix( BITMAP* bmp, int x, int y,
                           const moon& mn, double xval, double yval,
                           double blend )
{
  const double thetax = asin (xval) * 180 / PI;
  const double thetay = acos (yval) * 180 / PI;

  const double offset = (perlin2DPoint (1.0, mn.smoothness,
                                        mn.xoffset + mn.x + thetax,
                                        mn.yoffset + mn.y + thetay,
                                        mn.lambda, mn.octaves) + 1) / 2;
  const double percentVal = (perlin2DPoint (1.0, mn.smoothness,
                             mn.xoffset + mn.x * 1000 + thetax,
                             mn.yoffset + mn.y * 1000 + thetay,
                             mn.lambda, mn.octaves) + 1) / 2;

  set_add_blender (0, 0, 0, (int)(blend * xval * percentVal * offset * 255));
#ifdef THREADS
  drawing_mode (DRAW_MODE_TRANS, NULL, 0, 0);
#endif
  putpixel (bmp, x, y, mn.col1);
  set_add_blender (0, 0, 0, (int)(blend * xval * (1 - percentVal) * offset * 255));
  putpixel (bmp, x, y, mn.col2);
#ifdef THREADS
  solid_mode();
#endif
}



/*****************************************************************************
draw_amoon

Renders a single moon onto a bitmap.  Assumes transparent drawing is enabled.
Abeys the given bounding box, which may be smaller than the moon itself.
Uses the darkside parameter to decide which side of the moon should be
dark.  Obeys and updates the z-buffer.

The current implementation of this function is beggins for some
simplifications.  And again, what about thoes [xy]offset variables?
*****************************************************************************/
static void draw_amoon( BITMAP* bmp,
                        const moon& mn, int x0, int y0, int x1, int y1,
                        bool darkside, ZBuffer& zbuffer )
{
  for ( int y = y0; y != y1; ++y )
    {
      bool hityet = false;

      for ( int x = x0; x != x1; ++x )
        {
          /* Occupied? */
          if ( zbuffer.test(x,y) )
            continue ;

          /* Find distance from this moon */
          int xdist = mn.x - x;
          int ydist = mn.y - y;

          /* Compute some other nice circle values */
          const double fradius = (double) mn.radius ;
          double xval = (double)xdist / fradius;
          double yval = (double)ydist / fradius;
          double distance2 = (xdist * xdist) + (ydist * ydist);
          double distance = sqrt (distance2);

          /* A bound check -> are we in the circle? */
          if ( distance > fradius + 1 )
            {
              if ( hityet )	/* If we've already been inside at this y... */
                break;		/* then skip ahead to the next y */
              else			/* Otherwise... */
                continue ;	/* Stay at this y, and skip to the next x */
            }

          /* Edges use lighter blending */
          const double edgeval = coverage( distance, fradius );

          /* Now, should we paint this side of the moon? */
          if ( xval && ((xval<0) == darkside) )
            {
              paint_moonpix(
                bmp, x, y,
                mn, fabs(xval), yval,
                //fract_clamp( fabs(xval) ),
                //fract_clamp( fabs(yval) ),
                edgeval ) ;
            }

          /* Mark this pixel as occupied */
          zbuffer.set(x,y);
          hityet = true ;
        }
    }
}



/*****************************************************************************
draw_moons

Renders a set of moons over a given bitmap.  The bitmap to draw of and the
appropriate dimensions must be given.
*****************************************************************************/
void draw_moons (BITMAP* bmp, int width, int height)
{
  const bool darkside = rand() > (RAND_MAX/2+1) ;
  ZBuffer	zbuffer( width, height ) ;

  drawing_mode (DRAW_MODE_TRANS, NULL, 0, 0);
  for ( int numMoons = (int)central_rand( 14.0 );
        numMoons; --numMoons )
    {
      int x0, y0, x1, y1;

      /* Make up a moon */
      const moon mn = generate_moon( width, height );

      /* Where is it? */
      x0 = clamped_int( mn.x - mn.radius -1, 0, width ) ;
      y0 = clamped_int( mn.y - mn.radius -1, 0, height ) ;
      x1 = clamped_int( mn.x + mn.radius +1, 0, width ) ;
      y1 = clamped_int( mn.y + mn.radius +1, 0, height ) ;

      /* Draw it */
      draw_amoon( bmp, mn, x0, y0, x1, y1, darkside, zbuffer );
    }
  solid_mode ();
}



/*****************************************************************************
generate_sky

Given some input parameters, renders a sky (with moons) onto a bitmap.
*****************************************************************************/
void generate_sky (GLOBALDATA *global, BITMAP* bmp, const gradient* grad, int flags )
{
  double messiness = (rand () % 100 / 1000.0 + 0.05);
  const int xoffset = rand( );	/* For perlin, random starting x */
  const int yoffset = rand( );	/* For perlin, random starting y */

  for (int x = 0; x < global->screenWidth; x++)
    {
      for (int y = 0; y < global->screenHeight - MENUHEIGHT; y++)
        {
          double offset = 0;

          if ( flags & GENSKY_DETAILED )
            offset += perlin2DPoint (1.0, 200, xoffset + x, yoffset + y, 0.3, 6) * ((global->screenHeight - MENUHEIGHT) * messiness);

          if ( flags & GENSKY_DITHERGRAD )
            offset += rand () % 10 - 5;

          while (y + offset < 0)
            offset /= 2;
          while (y + offset + 1 > (global->screenHeight - MENUHEIGHT))
            offset /= 2;

          #ifdef THREADS
          solid_mode();
          #endif
          putpixel (bmp, x, y,
                    gradientColorPoint (grad, global->screenHeight - MENUHEIGHT, y + offset));
          #ifdef THREADS
          drawing_mode(global->env->current_drawing_mode, NULL, 0, 0);
          #endif

        }
    }
  draw_moons (bmp, global->screenWidth, global->screenHeight);
}



// This function should be a seperate thread which constantly generates
// skies in the background so as to not to hang the game after the
// buying screen.
void *Generate_Sky_In_Background(void *new_env)
{
   ENVIRONMENT *env = (ENVIRONMENT *) new_env;
   GLOBALDATA *my_global = env->Get_Globaldata();
   BITMAP *sky_in_progress = NULL;

   // do this constantly
   while ( my_global->get_command() != GLOBAL_COMMAND_QUIT )
   {
      // create a bitmap in waiting, if none exists
      if (! env->get_waiting_sky())
      {
         sky_in_progress = create_bitmap( my_global->screenWidth, my_global->screenHeight - MENUHEIGHT);
         generate_sky (my_global, sky_in_progress, env->my_sky_gradients[my_global->cursky],
                   (my_global->ditherGradients ? GENSKY_DITHERGRAD : 0 ) |
                   (my_global->detailedSky ? GENSKY_DETAILED : 0 )  );

         #ifdef THREADS
         env->lock(env->waiting_sky_lock);
         #endif
         env->waiting_sky = sky_in_progress;
         #ifdef THREADS
         env->unlock(env->waiting_sky_lock);
         #endif
         sky_in_progress = NULL;
      }

      LINUX_SLEEP;
   }

   #ifdef THREADS
   pthread_exit(NULL);
   return NULL;         // we never hit this, but it keeps the compiler from complaining
   #else
   return NULL;
   #endif
}

