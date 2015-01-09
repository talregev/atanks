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
#include "explosion.h"
#include "missile.h"
#include "decor.h"
#include "tank.h"


MISSILE::~MISSILE ()
{
  _env->removeObject (this);
  _global = NULL;
  _env    = NULL;
  weap    = NULL;
}

MISSILE::MISSILE (GLOBALDATA *global, ENVIRONMENT *env, double xpos, double ypos, 
                  double xvel, double yvel,
                  int weaponType):PHYSICAL_OBJECT(),weap(NULL)
{
  setEnvironment (env);
  player = NULL;
  _align = LEFT;
  _global = global;
  #ifdef NETWORK
  char buffer[256];
  sprintf(buffer, "MISSILE %d %d %lf %lf %d", (int)xpos, (int)ypos,
                  xvel, yvel, weaponType);
  global->Send_To_Clients(buffer);
  #endif
  x = xpos;
  y = ypos;
  xv = xvel;
  yv = yvel;
  type = weaponType;
  if (type < WEAPONS)
    weap = &weapon[type];
  else
    weap = &naturals[type - WEAPONS];
  mass = weap->mass;
  drag = weap->drag;
  sound = weap->sound;
  expSize = weap->radius;
  etime = weap->etime;
  damage = weap->damage;
  eframes = weap->eframes;
  picpoint = weap->picpoint;
  noimpact = weap->noimpact;
  countdown = -1;
  _bitmap = (BITMAP *)_global->missile[picpoint];
  physics = 0;
  age = 0;
  destroy = FALSE;
  if (type >= SML_METEOR && type <= LRG_METEOR)
    {
      angle = rand () % 360;
      spin = rand () % 20 - 10;
    }
  else
    {
      angle = 0;
      spin = 0;
    }

  // Napalm grows, others do not:
  if (type == NAPALM_JELLY)
    {
     bIsGrowing = true;
     iGrowRadius= 1;
    }
  else
    {
     bIsGrowing = false;
     iGrowRadius= expSize;
    }
  if ( (type == FUNKY_BOMBLET) || (type == FUNKY_DEATHLET) )
  {
      int temp_number = rand() % 5;
      switch (temp_number)
      {
         case 0: funky_colour = makecol(200, 0, 0); break;
         case 1: funky_colour = makecol(0, 200, 0); break;
         case 2: funky_colour = makecol(0, 0, 200); break;
         case 3: funky_colour = makecol(200, 200, 0); break;
         case 4: funky_colour = makecol(200, 0, 200); break;
      }
  }
}

void MISSILE::initialise ()
{
  PHYSICAL_OBJECT::initialise ();
  physics = 0;
  age = 0;
  if (type >= SML_METEOR && type <= LRG_METEOR)
    {
      angle = rand () % 360;
      spin = rand () % 20 - 10;
    }
  else
    {
      angle = 0;
      spin = 0;
    }
}



int MISSILE::triggerTest ()
{
  int quell = 1;
  TANK *tank;
  double old_delta_x = xv;

  // Has it hit a tank?
  for (int objCount = 0; (tank = (TANK*)_env->getNextOfClass (TANK_CLASS, &objCount)) && tank; objCount++)
    {
      if (x > tank->x - TANKWIDTH && x < tank->x + TANKWIDTH && y > tank->y && y < tank->y + TANKHEIGHT && tank->l > 0)
        {
          hitSomething = 1;
          tank->requireUpdate ();
        }
    }
  if (noimpact)
    {
      quell = 1;
    }
  else if ((y > 0) && (hitSomething || (y >= _global->screenHeight) || (getpixel (_env->terrain, (int)x, (int)y) != PINK)))
    // else if ( (y > 0) && (hitSomething || (y >= _global->screenHeight) || (_env->surface[(int)x] <= y) ) )
    {
      quell = 0;
      if (type >= SML_ROLLER && type <= DTH_ROLLER && !physics)
        {
          /* rollerupdate */
          if (age > 1)
            {

              quell = 1;
              physics = 1;
              y -= 5;
              if (x < 1)
                x = 1;
              if (x > (_global->screenWidth - 2))
                x = (_global->screenWidth - 2);

              if ( (y >= _env->surface[(int)x])       // y is surface or below
                   && (y <= _env->surface[(int)x] + 2) )  // but not burried more than 2 px
                y = _env->surface[(int)x] - 1;

              yv = 0.0;
              // if (xv > 0.0) yv = 1.0;
              // if (xv < 0.0) yv =-1.0;

              xv = 0.0;
              if (x == 1)
                  xv++;
              else if (x == (_global->screenWidth - 2) )
                  xv--;
              else
              {
                  int can_go_left = FALSE, can_go_right = FALSE;
                  if (getpixel (_env->terrain, (int)x + 1, (int)y + 1) == PINK)
                     can_go_right = TRUE;
                  if (getpixel (_env->terrain, (int)x - 1, (int)y + 1) == PINK)
                     can_go_left = TRUE;

                  if ( (can_go_left) && (!can_go_right) )
                  {
                     xv--;
                  }
                  else if ( (! can_go_left) && (can_go_right) )
                  {
                     xv++;
                  }
                  else if ( (can_go_left) && (can_go_right) )
                  {
                     xv = old_delta_x;      // if both are open, continue on old course
                  }
              }
              if (xv == 0.0)     // nothing worked, try something!
              {
                xv = yv;
              }
              yv = 0.0;
              hitSomething = 0;
            }
          else
            {
              physics = 4;
            }
        }
      else if (type == BURROWER || type == PENETRATOR)
        {
          if (physics == 1)
            {
              if (!hitSomething)
                quell = 1;
            }
          if (!physics)
            {
              physics = 1;
              xv *= 0.1;
              yv *= 0.1;
              quell = 1;
            }

        }
      // our weapon has sub weapons
      else if (weap->submunition >= 0)
        {
          WEAPON *submunition = &weapon[weap->submunition];

          quell = 1;
          if (weap->numSubmunitions > 0)
            {
              double divergenceStep = weap->divergence / (weap->numSubmunitions - 1);
              int startPoint;
              int randStart = rand () % 1000000;
              int submunitionPhys = 0;
              double inheritedXV = weap->impartVelocity * xv;
              double inheritedYV = weap->impartVelocity * yv;

              if (type == FUNKY_BOMB || type == FUNKY_DEATH)
                submunitionPhys = 1;

              if (divergenceStep < 0)
                {
                  startPoint = 0;
                }
              else
                {
                  startPoint = 180;
                }
              for (int spreadCount = 0; spreadCount < weap->numSubmunitions; spreadCount++)
                {
                  MISSILE *newmis;
                  double launchSpeed;
                  int newmisCount;
                  int dAngle;

                  dAngle = (int) (startPoint - (weap->divergence / 2) + (divergenceStep * spreadCount));
                  if (weap->spreadVariation > 0)
                    {
                      double variation = Noise (randStart + 1054 + spreadCount) * weap->spreadVariation;
                      dAngle += (int)(weap->divergence * variation);
                    }
                  while (dAngle < 0)
                    dAngle += 360;
                  while (dAngle >= 360)
                    dAngle -= 360;

                  newmisCount = submunition->countdown;
                  if (submunition->countVariation > 0)
                    {
                      double variation = Noise (randStart + 78689 + spreadCount) * submunition->countVariation;
                      newmisCount += (int)(submunition->countdown * variation);
                      if (newmisCount <= 0)
                        newmisCount = 1;
                    }

                  launchSpeed = weap->launchSpeed;
                  if (weap->speedVariation > 0)
                    {
                      double variation = Noise (randStart + 124786 + spreadCount) * weap->speedVariation;
                      launchSpeed += weap->launchSpeed * variation;
                    }

                  newmis = new MISSILE (_global, _env, x, y - 20, _global->slope[dAngle][0] * launchSpeed * (100.0 / _global->frames_per_second) + inheritedXV, _global->slope[dAngle][1] * launchSpeed * (100.0 / _global->frames_per_second) + inheritedYV, weap->submunition);

                  if (newmis)
                  {

                      newmis->physics = submunitionPhys;
                      newmis->player = player;
                      newmis->countdown = newmisCount;
                      newmis->noimpact = weapon[weap->submunition].noimpact;
                      newmis->setUpdateArea
                       ((int) newmis->x - 20,
                       (int) newmis->y - 20, 40, 40);
                  }
                  else
                      perror ( "missile.cc: Failed allocating memory for newmis in MISSILE::triggerTest (CLUSTER)");
                }
            }
          destroy = TRUE;
        }
    }

  if (countdown >= 0 && age >= countdown)
    {
      quell = 0;
    }
  if (type >= RIOT_CHARGE && type <= RIOT_BLAST)
    {
      quell = 0;
      destroy = TRUE;
    }
  if (!quell)
    {
      _env->realm--;
      trigger ();
    }


  return (!quell);
}

int MISSILE::applyPhysics ()
{
  TANK *ltank;
  int detonate = FALSE;
  int max_age;

  age++;

  // meteors live less time than regular missles
  if ( (type >= SML_METEOR) && (type <= LRG_METEOR) )
    max_age = MAX_METEOR_AGE;
  else
    max_age = MAX_MISSLE_AGE;

  // Napalm grows first:
  if (bIsGrowing)
    {
      if (age < max_age)
        {
          iGrowRadius = (int)round((double)expSize * ((double)age / (double)max_age));
          if (iGrowRadius < 2)
            iGrowRadius = 2;
        }
      else
        bIsGrowing = false; // Finished growing!
    }

  if ( (age > max_age * _global->frames_per_second) || (y < -65535) )
    detonate = 1;
  if (!physics)
    {
      if (type >= SML_METEOR && type <= LRG_METEOR)
        {
          angle += spin;
          angle = angle % 360;
        }
      else
        {
          angle = (int) (atan (yv / xv) * (180 / PI) * ACHANGE) - 64 + ((xv < 0) ? 128 : 0);
        }
      for (int objCount = 0; (ltank = (TANK*)_env->getNextOfClass (TANK_CLASS, &objCount)) && ltank; objCount++)
        {
          if (ltank->player != player)
            {
              double xaccel = 0, yaccel = 0;
              ltank->repulse (x + xv, y + yv, &xaccel, &yaccel, type);
              xv += xaccel;
              yv += yaccel;
            }
        }
      if ( PHYSICAL_OBJECT::applyPhysics () )
      {
        detonate = 1;
      }
      if (! detonate)
        detonate = checkPixelsBetweenPrevAndNow ();

      // mirvs trigger above ground
      if ( (! detonate) && ( (type == SMALL_MIRV) || (type == CLUSTER_MIRV) ) )
        {
          int above_ground = Height_Above_Ground();
          if ( (above_ground > 0) && (above_ground < TIGGER_HEIGHT) &&
               (yv > 0) )
            detonate = 1;
        }

      // if (! detonate &&
      if ( (type == BURROWER || type == PENETRATOR) )
        {
          hitSomething = 0;
          if ( ( y >= _global->screenHeight) &&
               ( (_env->current_wallType == WALL_STEEL) || (_env->current_wallType == WALL_WRAP) ) )
            {
              detonate = TRUE;
              hitSomething = 1;
              y = _global->screenHeight;
              yv = 0;
            }
        }
      if (rand () % 5 == 0)
        {
          DECOR *decor;
          decor = new DECOR (_global, _env, x, y, xv, yv, expSize / 20, DECOR_SMOKE);
          if (!decor)
            {
              perror ( "missile.cc: Failed allocating memory for decor in MISSILE::applyPhysics (METEOR)");
              // exit (1);
            }
        }
    }
  else
    {
      if (type >= SML_ROLLER && type <= DTH_ROLLER)
        {
          /* rollerupdate */
          // check whether we have hit anything
          if (	 (x < 2)
                ||(x > (_global->screenWidth - 3))
                ||(y > (_global->screenHeight - 3))
                ||(getpixel(_env->terrain, (int)x, (int)y) != PINK))
            detonate = 1;
          else
            {
              // roll roller
              float fSurfY = (float)_env->surface[(int)x] - 1;
              if ((fSurfY > y) && (y < (_global->screenHeight - 5)))
                {
                  if (fSurfY < (y + 5))
                    y = fSurfY;
                  else
                    y+=5;
                  if (xv > 0.0) 		angle += 3;
                  if (xv < 0.0) 		angle -= 3;
                  if (angle < 0)		angle += 256;
                  if (angle > 255)	angle -= 256;
                }
              else
                {
                  if (xv > 0.0)
                    {
                      angle += 3;
                      x++;
                    }
                  else
                    {
                      angle -= 3;
                      x--;
                    }
                  if (angle < 0)
                    angle += 256;
                  if (angle > 255)
                    angle -= 256;
                  if	(y >= MENUHEIGHT)
                    {
                      if (fSurfY > y)
                        y++;
                      else if (fSurfY >= (y - 2))
                        y = fSurfY - 1;
                    }
                }
              if (y > (_global->screenHeight - 5) && !detonate)
                y = (_global->screenHeight - 5);
            }
        }
      else if (type == FUNKY_BOMBLET || type == FUNKY_DEATHLET)
        {
          //double accel = (_env->wind - xv) / mass * drag;
          if (x + xv < 1 || x + xv > (_global->screenWidth-1))
            {
              xv = -xv;	//bounce on the border
            }
          else
            {
              x += xv;
            }

          // hit screen bottom then change the y velocity direction
          if (y + yv >= _global->screenHeight)
            {
              // only bounce if the wall is rubber
              if ( _env->current_wallType == WALL_RUBBER )
                {
                  yv = -yv * 0.5;
                  xv *= 0.95;
                }
              // bounce back with more force
              else if ( _env->current_wallType == WALL_SPRING )
                {
                  yv = -yv * SPRING_CHANGE;
                  xv *= 1.05;
                }
              // steel or wrap floor, detonate
              else
                {
                  y = _global->screenHeight;
                  yv = 0;
                  detonate = TRUE;
                }
            }
          else if (y+yv < 0) //hit screen top
            {
              yv = -yv * 0.5;
              xv *= 0.95;
            }
          y += yv;
        }
      else if (type == BURROWER || type == PENETRATOR)
        {
          angle = (int) (atan (yv / xv) * (180 / PI) * ACHANGE) - 64 + ((xv < 0) ? 128 : 0);
          for (int objCount = 0; (ltank = (TANK*)_env->getNextOfClass (TANK_CLASS, &objCount)) && ltank; objCount++)
            {
              if (ltank->player != player)
                {
                  double xaccel = 0, yaccel = 0;
                  ltank->repulse (x + xv, y + yv, &xaccel, &yaccel, type);
                  xv += xaccel * 0.1;
                  yv += yaccel * 0.1;
                }
            }
          if (x + xv < 1 || x + xv > (_global->screenWidth-1))
            {
              // if the wall is rubber, then bouce
              if ( _env->current_wallType == WALL_RUBBER )
                xv = -xv;	//bounce on the border
              // bounce with more force
              else if ( _env->current_wallType == WALL_SPRING )
                xv = -xv * SPRING_CHANGE;
              // wall is steel, detonate
              else if ( _env->current_wallType == WALL_STEEL )
                detonate = TRUE;
              // wrap around to other side of the screen
              else if ( _env->current_wallType == WALL_WRAP )
                {
                  if (xv < 0)
                    x = _global->screenWidth - 1;
                  else
                    x = 1;
                }
            }
          if (y + yv >= _global->screenHeight)
            {
              yv = -yv * 0.5;
              xv *= 0.95;
            }
          else if (y+yv < 0)   //hit screen top
            {
              yv = -yv *0.5;
              xv *= 0.95;
            }
          y += yv;
          x += xv;
          yv -= _env->gravity * 0.05 * (100.0 / _global->frames_per_second);
          if (getpixel (_env->terrain, (int)x, (int)y) == PINK)
            {
              detonate = TRUE;
            }
        }
    }

  if (detonate)
    {
      hitSomething = 1;
      if (y <= MENUHEIGHT)
        y = MENUHEIGHT + 1;
    }
  else if ((y <= MENUHEIGHT) && _global->bIsBoxed)
    {
      detonate = 1;
      hitSomething = 1; // Sorry, no more ceiling-drops for rollers!
    }

  return (detonate);
}


void MISSILE::trigger ()
{
  EXPLOSION *explosion;

  if (type >= TREMOR && type <= TECTONIC)
    explosion = new EXPLOSION (_global, _env, x, y, xv, yv, type);
  else if (type >= RIOT_CHARGE && type <= RIOT_BLAST) 
    explosion = new EXPLOSION (_global, _env, x, y, xv, yv, type);
  else
    explosion = new EXPLOSION (_global, _env, x, y, type);

  if (!explosion)
    {
      perror ( "missile.cc: Failed allocating memory for explosion in MISSILE::trigger");
      // exit (1);
    }
  else
     explosion->player = player;



  if ((_env->current_wallType == WALL_WRAP) && (type < WEAPONS))
    {
      EXPLOSION *cSecondExplo = NULL;
      if (x < weapon[type].radius)
        {
          if   (   (type >= TREMOR && type <= TECTONIC)
                   ||(type >= RIOT_CHARGE && type <= RIOT_BLAST)   )
            cSecondExplo = new EXPLOSION (_global, _env, _global->screenWidth + x, y, xv, yv, type);
          else
            cSecondExplo = new EXPLOSION (_global, _env, _global->screenWidth + x, y, type);
          if (!cSecondExplo)
            {
              perror ( "missile.cc: Failed allocating memory for cSecondExplo in MISSILE::trigger");
              // exit (1);
            }
        }
      if (x > (_global->screenWidth - weapon[type].radius))
        {
          if   (   (type >= TREMOR && type <= TECTONIC)
                   ||(type >= RIOT_CHARGE && type <= RIOT_BLAST)   )
            cSecondExplo = new EXPLOSION (_global, _env, x - _global->screenWidth, y, xv, yv, type);
          else
            cSecondExplo = new EXPLOSION (_global, _env, x - _global->screenWidth, y, type);
          if (!cSecondExplo)
            {
              perror ( "missile.cc: Failed allocating memory for cSecondExplo in MISSILE::trigger");
              // exit (1);
            }
        }
      if (cSecondExplo)
        cSecondExplo->player = player;
    }



  destroy = TRUE;

  if ((type >= DIRT_BALL && type <= SMALL_DIRT_SPREAD) || (type >= RIOT_CHARGE && type <= RIOT_BLAST))
    {
      play_sample ((SAMPLE *) _global->sounds[9], _env->scaleVolume(128 + (expSize / 2)), 128, 1000 - (expSize * 2), 0);
    }
  else if (type == NAPALM_JELLY)
    {
      // TODO?
      //play_sample ((SAMPLE *) _global->SOUND[9].dat, 128 + (expSize / 2), 128, 1000 - (expSize * 2), 0);
    }
  else
    {
      play_sample ((SAMPLE *) _global->sounds[WEAPONSOUNDS + sound], _env->scaleVolume(255), 128, 1000, 0);
    }
}

void MISSILE::draw (BITMAP *dest)
{
  if (!destroy)
    {
      // draw missile if it is above the screen
      if (y < MENUHEIGHT)
        {
          BITMAP *bbitmap = _bitmap;
          double by = y;
          int bangle = angle;

          _bitmap = (BITMAP*)_global->misc[3];
          y = (double)MENUHEIGHT + (_bitmap->h / 2);
          angle = 0;

          VIRTUAL_OBJECT::draw (dest);
          _bitmap = bbitmap;
          y = by;
          angle = bangle;
        }

      // draw missile on the screen
      else
        {

          if (type == NAPALM_JELLY)
            {
              if (bIsGrowing)
                {
                  circlefill (_env->db, (int)x, (int)y, iGrowRadius - 2, makecol (255, 0, 0));
                  circle(_env->db, (int)x, (int)y, iGrowRadius - 1, makecol(255, 150, 0));
                  circle(_env->db, (int)x, (int)y, iGrowRadius, makecol(255, 150, 0));
                  setUpdateArea ( (int)x - (iGrowRadius + 1),
                                  (int)y - (iGrowRadius + 1),
                                  (iGrowRadius + 1) * 2,
                                  (iGrowRadius + 1) * 2);
                }
              else
                {
                  circlefill (_env->db, (int)x, (int)y, expSize - 2, makecol (255, 0, 0));
                  circle(_env->db, (int)x, (int)y, expSize - 1, makecol(255, 150, 0));
                  circle(_env->db, (int)x, (int)y, expSize, makecol(255, 150, 0));
                  setUpdateArea ((int)x - (expSize + 1), (int)y - (expSize + 1), (expSize + 1) * 2, (expSize + 1) * 2);
                }
              requireUpdate ();
            }   // end of napalm

          // try drawing a funky bomblet
          else if ( ( type == FUNKY_BOMBLET) || (type == FUNKY_DEATHLET) )
          {
              circlefill(_env->db, (int) x, (int) y, 4, funky_colour );
              circle(_env->db, (int) x, (int) y, 5, makecol(0, 0, 0) );
              setUpdateArea( (int) x - 10, (int) y - 10, 20, 20);
              requireUpdate();
          } 
          // draw anything else, besides napalm
          else
            {
              VIRTUAL_OBJECT::draw (dest);
            }
        }
    }
}


void MISSILE::setEnvironment(ENVIRONMENT *env)
    {
      if (!_env || (_env != env))
        {
          _env = env;
          _index = _env->addObject (this);
        }
    }



int MISSILE::isSubClass (int classNum)
{
  if (classNum == MISSILE_CLASS)
    return (TRUE);
  else
    return (FALSE);
  //return (PHYSICAL_OBJECT::isSubClass (classNum));
}


// This function returns the distance above ground of
// the missile.
int MISSILE::Height_Above_Ground()
{
  int height = (int)y + 1;

  if (y < 0)
    return height;

  height = _env->surface[(int) x] - y;
  return height;
}



// Check to see if any tanks have SDI defense. If they
// do, then see if this missile should be shot down.
// Returns the shooting tank if a shot is to be fired
// or NULL if no tank will shoot.
TANK *MISSILE::Check_SDI(GLOBALDATA *global)
{
    TANK *my_tank;
    int index;
    double distance;
    int chance;

    if (type == NAPALM_JELLY)     // can't shoot jelly
       return NULL;

    for (index = 0; index < global->numPlayers; index++)
    {
        if ( ( global->players[index] ) && ( global->players[index]->tank) )
        {
            my_tank = global->players[index]->tank;
            if ( (my_tank->player->ni[ITEM_SDI]) && (my_tank->player != this->player) )
            {
                 distance = pow(x - my_tank->x, 2);
                 distance += pow(y - my_tank->y, 2);
                 distance = sqrt(distance);
                 // only fire if within range and above the tank
                 if ( ( distance < SDI_DISTANCE ) && (my_tank->y > y) )
                 {
                      chance = rand() % 5;
                      if (! chance)
                      {
                         return my_tank;
                      }
                 }        // within range
            }      // we have SDI
        }       // end of we have a valid player/tank
       
    }           // finished going through all players
    
    return NULL;
}


