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
perlin.cc

Provides noise and interpolation functionality, as prototyped in main.h
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

#include "main.h"



/*****************************************************************************
Noise, Noise2D

A floating point pseudorandom number generator.  Given a seed input value,
returns a randomized double in the range [-1.0,+1.0] .  Maintains no state.

Noise2D requires and uses two integer parameters.
*****************************************************************************/
double Noise (int x)
{
  x = (x<<13) ^ x;
  return ( 1.0 - ((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff)
           / 1073741824.0);
}

double Noise2D (int x, int y)
{
  int n;

  n = x + y * 57;
  n = (n << 13) ^ n;
  return ( 1.0 - ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff)
           / 1073741824.0);
}



/*****************************************************************************
interpolate

Performs a cosine interpolation between two points.  Given two y values
and a distance between them, return the interpolated y.  x1 and x2 are the y
values (sorry, it's the best I could explain it).  i is the distance,
expressed as a percentage of the wave length, ie 0<=i<1 .
*****************************************************************************/
/*
double interpolate (double x1, double x2, double i)
{
	double ft = i * M_PI;
	double f = (1 - cos(ft)) * 0.5;

	return (x1 * (1 - f) + (x2 * f));
}
*/


double interpolate (double x1, double x2, double i)
{
  double ft = i * M_PI;
  double f = (1 - cos(ft)) * 0.5;
  double result = (x1 * (1 - f) + (x2 * f));

  if (std::isnan(x1) || std::isnan(x2))
    return 0.0;
  if (std::isnan(result))
    return (x1 + x2) / 2.0; /* fall back to linear interpolation */
  return result;
}




/*
*
* For a really good explanation of perlin noise (and where most of this
*   perlin code was adapted from) see:
*
* http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
*
* It must be good because I understood it <:-)
*
* - Tom Hudson
*
*/
double perlin2DPoint (double amplitude, double scale, double xo, double yo,
                      double lambda, int octaves)
{
  double	maxH = 0;
  double	h = 0;
  for (int iteration = 1; iteration <= octaves; iteration++)
    {
      double	zoom = scale / (iteration * iteration);
      double	fractX = xo / zoom;
      double	fractY = yo / zoom;
      double	h1 = Noise2D ((int)fractX, (int)fractY);
      double	h2 = Noise2D ((int)fractX + 1, (int)fractY);
      double	h3 = Noise2D ((int)fractX, (int)fractY + 1);
      double	h4 = Noise2D ((int)fractX + 1, (int)fractY + 1);

      double	xi = fractX - (int)fractX;
      double	yi = fractY - (int)fractY;

      double	i1 = interpolate (h1, h2, xi);
      double	i2 = interpolate (h3, h4, xi);
      double	i3 = interpolate (i1, i2, yi);

      h += amplitude * i3;
      maxH += amplitude;

      amplitude *= lambda;
    }

  // Normalised
  return (h / maxH);
}

double perlin1DPoint (double amplitude, double scale, double xo,
                      double lambda, int octaves)
{
  double	maxH = 0;
  double	h = 0;
  for (int iteration = 1; iteration <= octaves; iteration++)
    {
      double	zoom = scale / (iteration * iteration);
      double	fractX = xo / zoom;
      double	h1 = Noise ((int)fractX);
      double	h2 = Noise ((int)fractX + 1);
      double	i = fractX - (int)fractX;

      h += amplitude * interpolate (h1, h2, i);
      maxH += amplitude;

      amplitude *= lambda;
    }

  // Normalised
  return (h / maxH);
}

