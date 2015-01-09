#ifndef DECOR_DEFINE
#define DECOR_DEFINE

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


#include "main.h"
#include "physobj.h"
#include "environment.h"
#include "globaldata.h"

enum	decorTypes
{
  DECOR_SMOKE
};

class DECOR: public PHYSICAL_OBJECT
  {
  public:
    int	type;
    int	radius;
    int	color;

    /* --- constructor --- */
    inline DECOR (GLOBALDATA *global, ENVIRONMENT *env, double xpos, double ypos, double xvel, double yvel,
                  int maxRadius, int decorType):PHYSICAL_OBJECT()
    {
      type = decorType;
      initialise ();
      setEnvironment (env);
      player = NULL;
      _align = LEFT;
      _global = global;
      x = xpos;
      y = ypos;
      xv = xvel;
      yv = yvel;
      if (maxRadius <= 3)
        radius = 3;
      else
        radius = 3 + (rand () % (maxRadius - 3));
    }

    /* --- destructor --- */
    inline virtual ~DECOR ()
    {
      _env->removeObject (this);
      _global = NULL;
      _env    = NULL;
    }

    /* --- non-inline methods --- */

    /* --- inline methods --- */

    inline virtual void	draw (BITMAP *dest)
    {
      if (!destroy)
        {
          int iCalcRadius = radius;
          if (type == DECOR_SMOKE) iCalcRadius = (int)(iCalcRadius * (4.0 * age / maxAge)); // The older, the larger...
          drawing_mode (DRAW_MODE_TRANS, NULL, 0, 0);
          set_trans_blender (0, 0, 0, 255 - (255 * age / maxAge));
          circlefill (dest, (int)x, (int)y, iCalcRadius, color);
          drawing_mode (DRAW_MODE_SOLID, NULL, 0, 0);
          setUpdateArea ( (int)x - (iCalcRadius * 2) - 1,
                          (int)y - (iCalcRadius * 2) - 1,
                          iCalcRadius * 4 + 2,
                          iCalcRadius * 4 + 2);
          requireUpdate ();
        }
    }

    inline virtual int	applyPhysics ()
    {
      /** Update: Smoke will age faster, if the wind is stronger! **/
      int iAgeMod = 1;
      if (_env->wind && (type == DECOR_SMOKE))
        iAgeMod = (int)round(fabs(_env->wind / (_env->windstrength / 2.0))) + 1;
      /* This produces: (with max wind = 8)
       * wind = 1 : round(1 / (8 / 2)) + 1 = round(1 / 4) + 1 = 0 + 1 = 1 <-- normal aging
       * wind = 4 : round(4 / (8 / 2)) + 1 = round(4 / 4) + 1 = 1 + 1 = 2 <-- raised aging
       * wind = 6 : round(6 / (8 / 2)) + 1 = round(6 / 4) + 1 = 2 + 1 = 3 <-- fast aging
       * wind = 8 : round(8 / (8 / 2)) + 1 = round(8 / 4) + 1 = 2 + 1 = 3 <-- fast aging */
      age += iAgeMod;

      if (!physics)
        {
          double xaccel = (_env->wind - xv) / mass * drag * _env->viscosity;
          xv += xaccel;
          x += xv;
          if (x < 1 || x > (_global->screenWidth-1))
            {
              destroy = TRUE;
            }
          double yaccel = (-1 - yv) / mass * drag * _env->viscosity;
          yv += yaccel;
          if (y + yv >= _global->screenHeight)
            {
              yv = -yv * 0.5;
              xv *= 0.95;
            }
          y += yv;
        }
      if (age > maxAge)
        {
          destroy = TRUE;
        }

      return (hitSomething);
    }

    inline virtual void	initialise ()
    {
      PHYSICAL_OBJECT::initialise ();
      physics = 0;
      age = 0;
      if (type == DECOR_SMOKE)
        {
          int tempCol = 128 + (rand () % 64);
          color = makecol (tempCol, tempCol, tempCol);
          mass = 1 + ((double)(rand () % 5) / 10);
          drag = 0.9 + ((double)(rand () % 90) / 100);
          maxAge = 20 + (rand () % 90);
        }
    }

    inline virtual int	isSubClass (int classNum)
    {
      if (classNum == DECOR_CLASS)
        return (TRUE);
      else
        return (FALSE);
    }

    inline virtual int	getClass ()
    {
      return (DECOR_CLASS);
    }

    inline virtual void setEnvironment(ENVIRONMENT *env)
    {
      if (!_env || (_env != env))
        {
          _env = env;
          _index = _env->addObject (this);
        }
    }

  };

#endif
