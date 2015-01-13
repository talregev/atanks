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
#include "player.h"


EXPLOSION::~EXPLOSION()
{
    /* the immediate destruction of the background creates a need for a second dirt slide,
    or there are holes left
    (important for chain missiles to work properly!) */
    for (int z = 0; z <= (radius + 2); z++)
    {
        setSlideColumnDimensions(_global, _env, x + z, true);
        setSlideColumnDimensions(_global, _env, x - z, true);
    }
    _env->removeObject(this);
    weap = NULL;
    _global = NULL;
    _env = NULL;
}

EXPLOSION::EXPLOSION(GLOBALDATA *global, ENVIRONMENT *env, double xpos, double ypos,
                     int weaponType) : PHYSICAL_OBJECT(),weap(NULL)
{
    drag = 0.95;
    mass = 3;
    a = 1;
    peaked = 0;
    exclock = 500;
    bIsWeaponExplosion = true; // Exploding tanks set them to false themselves!
    setEnvironment(env);
    player = NULL;
    _align = LEFT;
    _global = global;
#ifdef NETWORK
    /*
  char buffer[256];
  sprintf(buffer, "EXPLOSION %d %d %d", (int) xpos, (int) ypos, weaponType);
  global->Send_To_Clients(buffer);
  */
#endif
    x = xpos;
    y = ypos;
    type = weaponType;
    if (type < WEAPONS)
        weap = &weapon[type];
    else
        weap = &naturals[type - WEAPONS];
    radius = weap->radius;
    etime = weap->etime;
    damage = weap->damage;
    eframes = weap->eframes;

    // make sure dirt appears on the screen, not above the playing area
    if ((type >= DIRT_BALL) && (type <= SMALL_DIRT_SPREAD))
    {
        if (y < DIRT_CEILING)
            y = DIRT_CEILING + radius;
    }
}

EXPLOSION::EXPLOSION(GLOBALDATA *global, ENVIRONMENT *env, double xpos, double ypos,
                     double xvel, double yvel, int weaponType) : PHYSICAL_OBJECT(),weap(NULL)
{
    initialise();
    setEnvironment(env);
    _align = LEFT;
    _global = global;
    x = xpos;
    y = ypos;
    xv = xvel;
    yv = yvel;
    angle = (int) (atan2(xv, yv) / PI * 180);
    while (angle < 0)
        angle += 360;
    while (angle > 360)
        angle -= 360;

    type = weaponType;
    if (type < WEAPONS)
        weap = &weapon[type];
    else
        weap = &naturals[type - WEAPONS];
    radius = weap->radius;
    etime = weap->etime;
    damage = weap->damage;
    eframes = weap->eframes;
}

void EXPLOSION::initialise()
{
    PHYSICAL_OBJECT::initialise();
    drag = 0.95;
    mass = 3;
    a = 1;
    peaked = 0;
    exclock = 500;
    bIsWeaponExplosion = true; // Exploding tanks set them to false themselves!
}

int EXPLOSION::applyPhysics()
{
    if (type == NAPALM_JELLY)
    {
        if (rand() % 300 == 0)
        {
            DECOR *decor;
            decor = new DECOR (_global, _env, x, y, xv, yv, radius / 2, DECOR_SMOKE);
            if (!decor)
            {
                perror("explosion.cpp: Failed allocating memory for decor in applyPhysics");
                // exit (1);
            }
            // If the dirt below is falling away, napalm can fall, too:
            if (getpixel(_env->terrain, x, y + 1) == PINK)
            {
                // yv = _env->gravity * (100.0 / FRAMES_PER_SECOND);
                yv = _env->gravity * (100.0 / _global->frames_per_second);
                PHYSICAL_OBJECT::applyPhysics();
            }
            else
                yv = 0.0;
        }
    }
    /* if (dispersing)
    {
        double accel = (_env->wind - xv) / mass * drag * _env->viscosity;
        xv += accel;
        x += xv;
        y += yv;
    }
  */
    return hitSomething;
}


void EXPLOSION::explode()
{
    int calcblow;
    calcblow = 1;

    /* to allow synchronous tank explosions, explosions need to know what they are */
    if (player && player->tank && (type < WEAPONS))
        bIsWeaponExplosion = true;
    else
        bIsWeaponExplosion = false;

    if (peaked == 1 && a <= EXPLOSIONFRAMES + 1)
    {
        exclock++;
        if (exclock > weap->etime)
        {
            exclock = 0;
            a++;
            requireUpdate();
        }
        if (a > EXPLOSIONFRAMES + 1)
            destroy = TRUE;
    }
    if (a <= EXPLODEFRAMES + 1)
    {
        TANK *tank;
        if (a == 1)
        {
            for (int index = 0; (tank = (TANK*) _env->getNextOfClass(TANK_CLASS, &index)) && tank; index++)
            {
                // is tank directly above explosion?
                if ((fabs (x - tank->x) < radius) && (y - tank->y >= 0))
                    tank->creditTo = player;
            }
        }
        if ((a == 1) && (type <= TECTONIC || type >= WEAPONS || type == PERCENT_BOMB || type == REDUCER))
        {
            for (int index = 0; (tank = (TANK*) _env->getNextOfClass(TANK_CLASS, &index)) && tank; index++)
            {
                double distance;

                if (type >= SHAPED_CHARGE && type <= CUTTER)
                {
                    double dXDistance = fabs(x - tank->x);
                    double dYDistance;
                    if (y > tank->y)
                        dYDistance = fabs(y - tank->y) - (TANKHEIGHT * (3.0 / 4.0)); // To include the bottom of the tank
                    else
                        dYDistance = fabs(y - tank->y) - (TANKHEIGHT * (3.0 / 8.0)); // The top has to be hit, but not only brushed

                    if (dYDistance < 0) dYDistance = 0;

                    if ((dYDistance  <= (radius / 20)) && (dXDistance  >= (TANKWIDTH / 2)))
                        distance = ABSDISTANCE(dXDistance, dYDistance, 0, 0);
                    else
                        distance = 2 * radius; // clear outside the explosion!
#ifdef DEBUG
                    if ((dXDistance < (radius + TANKHEIGHT / 2)) && (dYDistance < 25))
                    {
                        cout << endl << "Shape: " << radius << " x " << (radius / 20) << endl;
                        printf( "Tank  X = % 4d, Tank  Y = % 4d\n", (int)tank->x, (int)tank->y);
                        printf( "Explo X = % 4d, Explo Y = % 4d\n", (int)x, (int)y);
                        printf( "Dist  X = % 4d, Dist  Y = % 4d\n", (int)dXDistance, (int)dYDistance);
                        cout << "Distance: " << distance << endl;
                    }
#endif // DEBUG
                }
                else
                    distance = ABSDISTANCE(x, y, tank->x, tank->y);

                if (distance <= (radius + TANKHEIGHT/2) && tank->l > 0)
                {
                    _global->updateMenu = 1;

                    if (type == PERCENT_BOMB)
                        tank->damage = (int) ((tank->l + tank->sh) / 2) + 1;
                    else if (type == REDUCER)
                    {
                        if (tank->player->damageMultiplier > 0.1)
                            tank->player->damageMultiplier *= 0.75;
                    }
                    else if (player)
                        tank->damage = (int) ((float) damage * ((float) 1 - ((fabs(distance) / (float) radius) / 2)) * player->damageMultiplier);
                    // player is not used in weather attacks
                    else
                        tank->damage = (int) (float) damage * ((float) 1 - ((fabs(distance) / (float) radius) / 2));
                    tank->creditTo = player;
                    tank->applyDamage();
                }
            }
            if (type >= TREMOR && type <= TECTONIC)
            {
                angle = (int) (atan2(yv, xv) / PI * 180);
                if (angle < 0)
                    angle = angle + 360;
                angle = angle % 360;
            }
        }
        exclock++;
        if (exclock > weap->etime)
        {
            exclock = 0;
            a++;
            requireUpdate();
        }
        if (a >= EXPLODEFRAMES + 1 && !peaked)
            calcblow = 0;
    }
    if (!calcblow)
    {
        if (type >= SHAPED_CHARGE && type <= CUTTER)
        {
            ellipsefill(_env->terrain, (int)x, (int)y, radius, radius / 20, PINK);
            setUpdateArea((int)x - (radius + 1), (int)y - (radius / 20 + 1), (radius + 1) * 2, (radius / 20 + 1) * 2);

        }

        else if (type == DRILLER)
        {
            ellipsefill(_env->terrain, (int) x, (int) y, radius / 20, radius, PINK);
            setUpdateArea((int) x - (radius + 1), (int) y - (radius + 1), (radius + 1) * 2, (radius + 1) * 2);
        }

        else if (type <= LAST_EXPLOSIVE || type >= WEAPONS || type == PERCENT_BOMB)
        {

            if (type != NAPALM_JELLY)
            {
                circlefill(_env->terrain, (int)x, (int)y, radius, PINK);
                // too much?  setUpdateArea ((int)x - (radius + 1), (int)y - (radius + 1), (radius + 1) * 2, (radius + 1) * 2);
                // still too much! setUpdateArea ((int)x - radius, (int)y - radius, radius * 2, radius * 2);
                setUpdateArea((int)x - radius, (int)y - radius, (radius * 2) - 2, (radius * 2) - 2);
            }
        }
        else  if ((type >= RIOT_BOMB) && (type <= HVY_RIOT_BOMB))
        {
            circlefill(_env->terrain, (int)x, (int)y, radius, PINK);
            setUpdateArea((int)x - (radius + 1), (int)y - (radius + 1), (radius + 1) * 2, (radius + 1) * 2);
        }
        else if ((type >= RIOT_CHARGE) && (type <= RIOT_BLAST))
        {
            double sx = x - _global->slope[angle][0] * 15;
            double sy = y - _global->slope[angle][1] * 15;
            triangle(_env->terrain, (int) sx, (int) sy,
                     (int) (sx + _global->slope[(angle + 45) % 360][0] * radius),
                     (int) (sy + _global->slope[(angle + 45) % 360][1] * radius),
                     (int) (sx + _global->slope[(angle + 315) % 360][0] * radius),
                     (int) (sy + _global->slope[(angle + 315) % 360][1] * radius), PINK);
            setUpdateArea((int)sx - (radius + 1), (int)sy - (radius + 1), (radius + 1) * 2, (radius + 1) * 2);
        }
        else if ((type >= DIRT_BALL) && (type <= SMALL_DIRT_SPREAD))
        {
            BITMAP *tmp; //for mixing

            tmp = create_bitmap(radius * 2, radius * 2);
            clear_to_color(tmp, PINK);
            for (int count = 0; count < radius ; count++)
                circle(tmp, radius, radius, count, (player) ? player->color : makecol(0, 255, 0));
            setUpdateArea((int)x - (radius + 1), (int)y - (radius + 1), (radius + 1) * 2, (radius + 1) * 2);

            //copy terrain over explosion
            masked_blit(_env->terrain, tmp, (int)x - radius, (int)y - radius, 0, 0, radius*2, radius*2);

            //blit back exploded terrain
            masked_blit(tmp, _env->terrain, 0, 0,(int)x - radius, (int)y - radius, radius*2, radius*2);

            destroy_bitmap(tmp);
        }

        for (int z = 0; z < (_current.w + 2); z++)
            setSlideColumnDimensions (_global, _env, _current.x + z, TRUE);
        peaked = 1;
        dispersing = 1;
    }
}

void EXPLOSION::draw(BITMAP *dest)
{
    if (type >= SHAPED_CHARGE && type <= CUTTER)
    {
        if (a > 1 && a <= EXPLOSIONFRAMES + 1)
        {
            rotate_scaled_sprite(dest, (BITMAP *) _global->gfxData.flameFront[(a + (EXPLOSIONFRAMES * weap->eframes)) - 2],
                                 (int)x - radius, (int)y - (radius / 20), itofix (0), ftofix ((double) radius / 300));
            setUpdateArea((int)x - (radius + 1), (int)y - (radius / 20 + 1),
                          (radius + 1) * 2, (radius / 20 + 1) * 2);
        }
    }
    else if (type == DRILLER)
    {
        if (a > 1 && a <= EXPLOSIONFRAMES + 1)
        {
            rotate_scaled_sprite(dest, (BITMAP *) _global->gfxData.flameFront[(a + (EXPLOSIONFRAMES * weap->eframes)) - 2],
                                 (int) x - (radius), (int) y - (radius / 20), itofix(192), ftofix(radius / 300));
            // rotate_scaled_sprite(dest, (BITMAP *) _global->gfxData.flameFront[(a + (EXPLOSIONFRAMES * weap->eframes)) - 2], (int)x - radius, (int)y - (radius / 20), itofix (0), ftofix ((double) radius / 300));

            setUpdateArea((int) x - (radius + 1), (int) y - (radius + 1), (radius + 1) * 2, (radius + 1) * 2);
        }
    }
    else if (type == NAPALM_JELLY)
    {
        int blobSize = (int) (radius - ((double) a / EXPLOSIONFRAMES) * radius + 1);
        if (blobSize < 2)
            blobSize = 2;    // avoid circle size crash
        circlefill(_env->db, (int) x, (int) y, blobSize - 2, makecol(255, 0, 0));
        circle(_env->db, (int) x, (int) y, blobSize - 1, makecol(255, 150, 0));
        circle(_env->db, (int) x, (int) y, blobSize, makecol(255, 150, 0));
        setUpdateArea((int) x - (radius + 1), (int) y - (radius + 1), (radius + 1) * 2, (radius + 1) * 2);
    }
    else if (type <= LAST_EXPLOSIVE || type >= WEAPONS || type == PERCENT_BOMB)
    {
        if (a > 1 && a <= EXPLOSIONFRAMES + 1)
        {
            /* background needs to be cleard immediately to allow chain missiles to work
             * (and horizontal spreads look *far* better, too! ;) )
             * Note: Shaped charges are always shot alone, and they live off their visual effect.
             * Adding the immediate destruction on them, too, would cut off some of the effect... */
            if (a <= EXPLODEFRAMES)
                circlefill(_env->terrain, (int) x, (int) y, (int) ((radius / EXPLODEFRAMES) * a), PINK);
            rotate_scaled_sprite(dest, (BITMAP *) _global->gfxData.explosions[(a + (EXPLOSIONFRAMES * weap->eframes)) - 2],
                                 (int)x - radius, (int)y - radius, itofix (0), ftofix ((double) radius / 107));
            setUpdateArea((int) x - (radius + 1), (int) y - (radius + 1), (radius + 1) * 2, (radius + 1) * 2);
        }
    }
    else if ((type <= TECTONIC) && (type >= TREMOR))
    {
        if (a > 1 && a <= EXPLODEFRAMES + 1)
            drawFracture(_global, _env, _env->terrain, &_current, (int)x, (int)y, angle, (int)(((double)a / EXPLODEFRAMES) * (radius / 4)), radius, 5, 0);
    }
    else if ((type >= RIOT_BOMB) && (type <= HVY_RIOT_BOMB))
    {
        if (a > 1 && a <= EXPLODEFRAMES + 1)
        {
            int startCirc = (radius / EXPLODEFRAMES) * a;
            int colour = (player) ? player->color : makecol(0, 0, 255);
            circlefill(_env->terrain, (int) x, (int) y, startCirc, PINK);
            circle(dest, (int) x, (int) y, startCirc, colour);
            setUpdateArea((int) x - (radius + 1), (int)y - (radius + 1),
                          (radius + 1) * 2, (radius + 1) * 2);
        }
    }
    else if ((type >= RIOT_CHARGE) && (type <= RIOT_BLAST))
    {
        if (a > 1 && a <= EXPLODEFRAMES + 1)
        {
            double sx = x - _global->slope[angle][0] * 15;
            double sy = y - _global->slope[angle][1] * 15;
            int startCirc = (radius / EXPLODEFRAMES) * a;
            triangle(dest, (int) sx, (int) sy, (int) (sx + _global->slope[(angle + 45) % 360][0] * startCirc), (int)(sy + _global->slope[(angle + 45) % 360][1] * startCirc),(int)(sx + _global->slope[(angle + 315) % 360][0] * startCirc),(int)(sy + _global->slope[(angle + 315) % 360][1] * startCirc), player->color);
            setUpdateArea((int) sx - (startCirc + 1), (int) sy - (startCirc + 1), (startCirc + 1) * 2, (startCirc + 1) * 2);
        }
    }
    else
    {
        if (a > 1 && a <= EXPLODEFRAMES + 1)
        {
            int startCirc = (radius / EXPLODEFRAMES) * a;
            circlefill(dest, (int) x, (int) y, startCirc, (player)?player->color: makecol (0, 255, 0) );
            startCirc += (radius / EXPLODEFRAMES) * 2;
            setUpdateArea((int) x - startCirc, (int) y - startCirc, startCirc * 2, startCirc * 2);
        }
    }
}

int EXPLOSION::isSubClass(int classNum)
{
    if (classNum == EXPLOSION_CLASS)
        return TRUE;
    else
        return FALSE;
    //return (PHYSICAL_OBJECT::isSubClass (classNum));
}
