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


#include "physobj.h"
#include "environment.h"


void PHYSICAL_OBJECT::initialise ()
    {
      VIRTUAL_OBJECT::initialise ();
      hitSomething = 0;
    }


void PHYSICAL_OBJECT::draw (BITMAP *dest)
    {
      VIRTUAL_OBJECT::draw (dest);
    }



int  PHYSICAL_OBJECT::checkPixelsBetweenPrevAndNow ()
    {
      double startX = x - xv;
      double startY = y - yv;

      if (checkPixelsBetweenTwoPoints (_global, _env, &startX, &startY, x, y))
        {
          x = startX;
          y = startY;
          return (1);
        }

      return (0);
    }



/** @brief applyPhysics
  *
  * Moves the object according to momentum and wind, boundec off of walls/ceiling/floor, and checks
  * whether something is hit.
  */
int PHYSICAL_OBJECT::applyPhysics()
{
  // Normal physics

  double dPrevX = x;
  double dPrevY = y;
  if (((x + xv) <= 1) || ((x + xv) >= (_global->screenWidth - 1)))
    {
      if	(	(((x + xv) < 1) && checkPixelsBetweenTwoPoints (_global, _env, &dPrevX, &dPrevY, 1, y))
           ||(	((x + xv) > (_global->screenWidth - 2))
               && checkPixelsBetweenTwoPoints (_global, _env, &dPrevX, &dPrevY, (_global->screenWidth - 2), y))	)
        {
          x = dPrevX;
          y = dPrevY;
          hitSomething = true;
        }
      else
        {
          //motion - wind affected
          switch (_env->current_wallType)
            {
            case WALL_RUBBER:
              xv = -xv;	//bounce on the border
              break;
            case WALL_SPRING:
              xv = -xv * SPRING_CHANGE;
              break;
            case WALL_WRAP:
              if (xv < 0)
                x = _global->screenWidth - 2; // -1 is the wall itself
              else
                x = 1;
              break;
            case WALL_STEEL:
              x += xv;
              if (x < 1)
                x = 1;
              if (x > (_global->screenWidth - 2))
                x = _global->screenWidth - 2;
              xv = 0; // already applied!
              hitSomething = 1; // blow up on steel
              break;
            }
        }
    }
  if (!hitSomething)
    xv += (double)(_env->wind - xv) / mass * drag * _env->viscosity;

  // hit floor or boxed top
  if ((y + yv >= (_global->screenHeight - 1))
      ||((y + yv <= MENUHEIGHT) && (_global->bIsBoxed)))
    {
      if	(	(	_global->bIsBoxed && ((y + yv) <= MENUHEIGHT)
             && checkPixelsBetweenTwoPoints (_global, _env, &dPrevX, &dPrevY, x, MENUHEIGHT + 1))
           ||(	((y + yv) > (_global->screenHeight - 2))
               && checkPixelsBetweenTwoPoints (_global, _env, &dPrevX, &dPrevY, x, (_global->screenHeight - 2)))	)
        {
          x = dPrevX;
          y = dPrevY;
          hitSomething = true;
        }
      else
        {
          switch (_env->current_wallType)
            {
            case WALL_RUBBER:
              yv = -yv * 0.5;
              xv *= 0.95;
              break;
            case WALL_SPRING:
              yv = -yv * SPRING_CHANGE;
              xv *= 1.05;
              break;
            default:
              // steel or wrap floor (ceiling)
              y += yv;
              if (y >= (_global->screenHeight - 1))
                y = _global->screenHeight - 2; // -1 would be the floor itself!
              else
                y = MENUHEIGHT + 1; // +1 or it would be the wall itself
              yv = 0; // already applied!
              hitSomething = 1;
            }
          if (fabs(xv) + fabs(yv) < 0.8)
            hitSomething = 1;
        }
    }

  // velocity check:
  /** the old calculation was wrong, but the new one, taking FPS into account, eventually works! **/
  double dActVelocity = ABSDISTANCE((long double)xv, (long double)yv, 0, 0); // a²+b²=c² ... says Pythagoras :)
  double dMaxVelocity = _global->dMaxVelocity * (1.20 + ((double)mass / ((double)MAX_POWER / 10.0)));
  // apply mass, as more mass allows more max velocity:

  // The default means, that a small missile can be accelerated by 25% over MAX_POWER,
  // while a large Napalm Bomb can go up to 220%.

  if (dActVelocity > dMaxVelocity)
    {
#ifdef DEBUG
      cout << "** Missile too fast!" << endl;
      cout << "mass: " << mass << " - Max Velocity: " << dMaxVelocity << " (FPS: " << _global->frames_per_second << ")" << endl;
      cout << "xv: " << xv << " - yv: " << yv << " - Act Velocity: " << dActVelocity << endl;
#endif // DEBUG
      // apply *some* velocity, as the thing is killed on its way
      y += yv / (1.0 + ((double)(rand() % 40) / 10.0)); // This produces somthing
      x += xv / (1.0 + ((double)(rand() % 40) / 10.0)); // between 1.0 and 5.0
      xv = 0.0;
      yv = 0.0;
      if (y <= MENUHEIGHT)
        y = MENUHEIGHT + 1;
      if (y > (_global->screenHeight - 2))
        y = _global->screenHeight - 2;
      if ((x < 1) && (_env->current_wallType != WALL_WRAP))
        x = 1; // Wrapped Explosion takes care for explosions off the screen
      if ((x > (_global->screenWidth - 2)) && (_env->current_wallType != WALL_WRAP))
        x = _global->screenWidth - 2; // Wrapped Explosion takes care for explosions off the screen
      hitSomething = 1;
    }
  if (!hitSomething)
    {
      x += xv;
      y += yv;
      yv += _env->gravity * (100.0 / _global->frames_per_second);
      // Barrier test:
      if ( (yv <= -1.0) && (y <= (_global->screenHeight * -25.0)))
        yv *= -1.0;
    }
  else
    {
      // if something *is* hit, ensure that that damn thing is on screen! (only y matters here!)
      if ((y < (MENUHEIGHT + 1)) && _global->bIsBoxed)	y = MENUHEIGHT + 1;
      if ((y < MENUHEIGHT) && !_global->bIsBoxed)				y = MENUHEIGHT;
    }
  return (hitSomething);
}

/* --- global method --- */
int checkPixelsBetweenTwoPoints (GLOBALDATA *global, ENVIRONMENT *env, double *startX, double *startY, double endX, double endY)
{
  int landFound = 0;

  // only check if on screen
  double xDist = endX - *startX;
  double yDist = endY - *startY;
  double length;
  double xInc;
  double yInc;

  if ((fabs(xDist) < 2) && (fabs(yDist) < 2))
    {
      if	(	(endX > 0) 													// 0 is the wall!
           &&(endX < (global->screenWidth - 1))	// -1 is the wall!
           &&(endY < (global->screenHeight - 1))	// -1 is the floor!
           &&(endY > (MENUHEIGHT + 1))						// or it would always fail for high shots!
           &&(getpixel (env->terrain, (int)endX, (int)endY) != PINK)	)
        return (1);
      else
        return (0);
    }

  length = FABSDISTANCE(xDist, yDist, 0, 0);
  yInc = yDist / length;
  xInc = xDist / length;

  // sanity check
  if (length > (global->screenWidth + global->screenHeight))
    length = global->screenWidth + global->screenHeight;

  //check all pixels along line for land
  for (int lengthPos = 0; lengthPos < length; lengthPos++)
    {
      //found land
      if	(	(*startX > 0) 													// 0 is the wall!
           &&(*startX < (global->screenWidth - 1))		// -1 is the wall!
           &&(*startY < (global->screenHeight - 1))	// -1 is the floor!
           &&(*startY > (MENUHEIGHT + 1))						// or it would always fail for high shots!
           &&(getpixel(env->terrain, (int)*startX, (int)*startY) != PINK)	)
        {
          // Leaves startX and Y at current point if within range!
          landFound = 1;
          break;
        }
      *startX += xInc;
      *startY += yInc;
    }

  return (landFound);
}

