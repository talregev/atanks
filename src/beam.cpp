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

#include "environment.h"
#include "globaldata.h"
#include "physobj.h"
#include "player.h"
#include "decor.h"
#include "tank.h"
#include "beam.h"


BEAM::~BEAM ()
{
  requireUpdate();
  update();
  _env->make_bgupdate (_current.x, _current.y, _current.w, _current.h);
  _env->make_bgupdate (_old.x, _old.y, _old.w, _old.h);
  _env->removeObject (this);
  weap    = NULL;
  _global = NULL;
  _env    = NULL;
   delete [] points;
}

void tracePath (int *targetX, int *targetY, GLOBALDATA *global, ENVIRONMENT *env, int minRange, double xpos, double ypos, int angle)
{
  double dx = global->slope[angle][0];
  double dy = global->slope[angle][1];
  double tx = xpos, ty = ypos;
  int hitSomething = 0;
  int range = 0;

  while ((!hitSomething) && tx > -10 &&
         tx < global->screenWidth + 10 && ty > -10 &&
         ty < global->screenHeight &&
         (getpixel (env->terrain, (int)tx, (int)ty) == PINK))
    {
      TANK *ltank;
      for (int objCount = 0; (ltank = (TANK*)env->getNextOfClass (TANK_CLASS, &objCount)) && ltank; objCount++)
        {
          if (range > minRange && tx > ltank->x - TANKWIDTH && tx < ltank->x + TANKWIDTH && ty > ltank->y && ty < ltank->y + TANKHEIGHT && ltank->l > 0)
            {
              hitSomething = 1;
              ltank->requireUpdate ();
            }
        }
      tx += dx;
      ty += dy;
      range++;
    }
  *targetX = (int)tx;
  *targetY = (int)ty;
}

BEAM::BEAM (GLOBALDATA *global, ENVIRONMENT *env, double xpos, double ypos, int fireAngle,
            int weaponType):PHYSICAL_OBJECT(),weap(NULL),points(NULL)
{
  player = NULL;
  drag = 0.00;
  mass = 0;
  clock = 500;
  age = 0;
  setEnvironment (env);
  _align = LEFT;
  _global = global;
  #ifdef NETWORK
  char buffer[256];
  sprintf(buffer, "BEAM %d %d %d %d", (int) xpos, (int) ypos, fireAngle, weaponType);
  global->Send_To_Clients(buffer);
  #endif
  x = xpos;
  y = ypos;
  angle = fireAngle % 360;
  type = weaponType;
  if (type < WEAPONS)
    weap = &(weapon[type]);
  else
    weap = &(naturals[type - WEAPONS]);
  radius = weap->radius;


  if (type >= SML_LIGHTNING && type <= LRG_LIGHTNING)
    {
      damage = (double)weap->damage / (10 * (type - SML_LIGHTNING + 1));
      maxAge = 10 * (type - SML_LIGHTNING + 1);
      numPoints = 4 + rand () % 10;
      color = WHITE;
      play_sample ((SAMPLE *) _global->sounds[LIGHTNING_SOUND], env->scaleVolume(128 + (radius * 10)), 128, 500 + (radius * 50), 0);
    }
  else
    {
      numPoints = 2;
      if (type >= SML_LAZER && type <= LRG_LAZER)
        damage = (double)weap->damage / (10 * (type - SML_LAZER + 1));
      maxAge = 10 * (type - SML_LAZER + 1);
      color = makecol (255 - (type - SML_LAZER) * 64, 128, 64 + (type - SML_LAZER) * 64);
    }
  tracePath (&targetX, &targetY, global, env, radius + 2, x, y, angle);
  xv = targetX - x;
  yv = targetY - y;
  points = new int[numPoints * 2];
  if (!points)
    {
      perror ( "beam.cc: Failed allocating memory for points in BEAM::BEAM");
      // exit (1);
    }
  setLightningPath ();
}

void BEAM::setLightningPath ()
{
  int point = 0;
  double x1 = x, y1 = y;

  do
    {
      double offX, offY;
      if (point > 0 && point < numPoints - 1)
        {
          offX = (perlin2DPoint (1.0, 20, 12746 + x1, y1, 0.3, 6) + 1) * (radius * 10);
          offY = (perlin2DPoint (1.0, 20, x1, 1273 + y1, 0.3, 6) + 1) * (radius * 10);
        }
      else
        {
          offX = 0;
          offY = 0;
        }
      points[point * 2] = (int)(x1 + offX);
      points[point * 2 + 1] = (int)(y1 + offY);

      x1 += xv / (numPoints - 1);
      y1 += yv / (numPoints - 1);
      point++;
    }
  while (point < numPoints);
}

void BEAM::initialise ()
{
  PHYSICAL_OBJECT::initialise ();
  drag = 0.00;
  mass = 0;
  clock = 500;
  age = 0;
}

int BEAM::applyPhysics ()
{
  TANK *ltank;
  DECOR *decor;

  age++;
  circlefill (_env->terrain, targetX, targetY, radius, PINK);
  for (int col = targetX - radius - 1; col < targetX + radius + 1; col++)
    setSlideColumnDimensions (_global, _env, col, true);

  if (age > maxAge)
    destroy = TRUE;

  for (int objCount = 0; (ltank = (TANK*)_env->getNextOfClass (TANK_CLASS, &objCount)) && ltank; objCount++)
    {
      if (targetX > ltank->x - TANKWIDTH - radius && targetX < ltank->x + TANKWIDTH + radius && targetY > ltank->y - radius && targetY < ltank->y + TANKHEIGHT + radius && ltank->l > 0)
        {
          // hitSomething = 1;
          ltank->requireUpdate ();

          // make sure a player is involved
          if (player)
            ltank->damage += damage * player->damageMultiplier;
          else
            ltank->damage += damage;

          ltank->creditTo = player;
          if (destroy)
            ltank->applyDamage ();
        }
    }
  decor = new DECOR (_global, _env, targetX, targetY, rand () % 6 - 3, rand () % 6 - 3, radius, DECOR_SMOKE);
  if (!decor)
    {
      perror ( "beam.cc: Failed allocating memory for decor in applyPhysics");
      // exit (1);
    }
  tracePath (&targetX, &targetY, _global, _env, radius + 2, x, y, angle);
  setLightningPath ();

  return (hitSomething);
}

int beamRadius;
void lazerPoint (BITMAP *dest, int x1, int y1, int color)
{
  circlefill (dest, x1, y1, beamRadius, color);
}

void lightningPoint (BITMAP *dest, int x1, int y1, int age)
{
  double pointRad = (perlin2DPoint (1.0, 2, x1 + age, y1, 0.3, 6) + 1) / 2 * beamRadius + 1;
  double offX = (perlin2DPoint (1.0, 20, 127846 + x1, y1 + age, 0.3, 6) + 1) * beamRadius;
  double offY = (perlin2DPoint (1.0, 20, x1 + age, 12973 + y1, 0.3, 6) + 1) * beamRadius;

  circlefill (dest, x1 + (int)offX, y1 + (int)offY, (int)pointRad, WHITE);
}

void BEAM::draw (BITMAP *dest)
{
  int x1, y1, x2, y2;
  setUpdateArea ((int)x - radius, (int)y - radius, (int)x + radius * 2, (int)y + radius * 2);

  drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
  _env->current_drawing_mode = DRAW_MODE_TRANS;
  set_trans_blender (0, 0, 0, (int)50);
  if (type >= SML_LIGHTNING && type <= LRG_LIGHTNING)
    {
      beamRadius = radius;
      for (int point = 1; point < numPoints; point++)
        {

          x1 = points[(point - 1) * 2];
          y1 = points[(point - 1) * 2 + 1];
          x2 = points[point * 2];
          y2 = points[point * 2 + 1];

          do_line (dest, x1, y1, x2, y2, age, lightningPoint);
          addUpdateArea (MIN(x1,x2) - radius * 6,
                         MIN(y1,y2) - radius * 6,
                         MAX(x1,x2) + radius * 2 * 6,
                         MAX(y1,y2) + radius * 2 * 6);
        }
    }
  else if (type >= SML_LAZER && type <= LRG_LAZER)
    {
      beamRadius = radius;
      for (int point = 1; point < numPoints; point++)
        {

          x1 = points[(point - 1) * 2];
          y1 = points[(point - 1) * 2 + 1];
          x2 = points[point * 2];
          y2 = points[point * 2 + 1];

          do_line (dest, x1, y1, x2, y2, color, lazerPoint);
          //do_line (dest, x1, y1, x2, y2, color, lightningPoint);
          addUpdateArea (MIN(x1,x2) - radius * 2,
                         MIN(y1,y2) - radius * 2,
                         MAX(x1,x2) + radius * 2 * 2,
                         MAX(y1,y2) + radius * 2 * 2);
        }
    }
  drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
  _env->current_drawing_mode = DRAW_MODE_SOLID;
  requireUpdate ();
}

/*void BEAM::update ()
{
	_env->combineUpdates = FALSE;
	VIRTUAL_OBJECT::update ();
	_env->combineUpdates = TRUE;
}*/

int BEAM::isSubClass (int classNum)
{
  if (classNum == BEAM_CLASS)
    return (TRUE);
  else
    return (FALSE);
  //return (PHYSICAL_OBJECT::isSubClass (classNum));
}
