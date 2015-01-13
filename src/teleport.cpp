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
#include "teleport.h"

TELEPORT::~TELEPORT()
{
    requireUpdate();
    update();
    _env->make_bgupdate(_current.x, _current.y, _current.w, _current.h);
    _env->make_bgupdate(_old.x, _old.y, _old.w, _old.h);
    _env->removeObject(this);
    _env = NULL;
    _global = NULL;
    object = NULL;
    remote = NULL;
}

TELEPORT::TELEPORT(GLOBALDATA *global, ENVIRONMENT *env, VIRTUAL_OBJECT *targetObj, int destinationX, int destinationY,
                   int objRadius, int duration) : VIRTUAL_OBJECT(),clock(duration),startClock(duration),peaked(0),
    object(NULL),remote(NULL)
{
    setEnvironment(env);
    player = NULL;
    _align = LEFT;
    _global = global;
    object = targetObj;
    radius = objRadius;
    remote = new TELEPORT(global, env, this, destinationX, destinationY);
    if (!remote)
        perror("teleport.cpp: Failed allocating memory for remote in TELEPORT::TELEPORT");

    play_sample((SAMPLE *) _global->sounds[item[ITEM_TELEPORT].sound], _env->scaleVolume(255), 128, 1000, 0);

#ifdef NETWORK
    // this seems to be the teleport we usually use
    int player_index = 0;
    bool found = false;
    TANK *the_tank = (TANK*) targetObj;
    // match the player with the tank
    while ((player_index < global->numPlayers) && !found)
    {
        if ((global->players[player_index]->tank) && (global->players[player_index]->tank == the_tank))
            found = true;
        else
            player_index++;
    }
    if (found)
    {
        char buffer[64];

        sprintf(buffer, "TELEPORT %d %d %d", player_index, destinationX, destinationY);
        global->Send_To_Clients(buffer);
    }
#endif
}

TELEPORT::TELEPORT(GLOBALDATA *global, ENVIRONMENT *env, TELEPORT *remoteEnd, int destX, int destY) : VIRTUAL_OBJECT(),
                   clock(remoteEnd->startClock),startClock(remoteEnd->startClock),peaked(0)
{
    remote = remoteEnd;
    setEnvironment(env);
    player = NULL;
    _align = LEFT;
    _global = global;
    object = NULL;
    x = destX;
    y = destY;
    radius = remote->radius;
}

void TELEPORT::initialise()
{
    VIRTUAL_OBJECT::initialise();
    peaked = 0;
    clock = startClock;
}

int TELEPORT::applyPhysics()
{
    if (object)
    {
        x = object->x;
        y = object->y;
    }
    else
        clock = remote->clock;

    if (clock < 1)
    {
        if (clock == 0)
        {
            if (object)
            {
                object->x = remote->x;
                object->y = remote->y;
                remote->object = object;
                object = NULL;
                remote->clock--;
            }
        }
        if (clock < -startClock / 2)
            destroy = TRUE;
    }
    clock--;
    return 0;
}


// new teleport version
void TELEPORT::draw(BITMAP *dest)
{
    BITMAP *tempBitmap;
    double pClock = clock;
    int blobSize, pRadius, maxblobs;

    if (pClock < 1.0)
        pClock = 1.0 + (1.0 - (pClock * 2.0));
    blobSize = 8 - (int) round(8 / (startClock / pClock)) + 1;
    pRadius  = radius - (int) round(radius / (startClock / pClock)) + 1;
    maxblobs = 1 + (pRadius * 4);

    tempBitmap = create_bitmap(radius * 2, radius * 2);
    blit(dest, tempBitmap, remote->x - radius, remote->y - radius, 0, 0, radius * 2, radius * 2);

    if (object)
        remote->draw(dest);

    drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
    set_trans_blender(0, 0, 0, 255 - (int) ((pClock / startClock) * 255));
    for (int count = (int) round(maxblobs + pClock); count > pClock; count--)
    {
        int xOff = (int) (perlin2DPoint(1.0, 200, 1278 + x + count * 100, pClock, 0.25, 6) * pRadius);
        int yOff = (int) (perlin2DPoint(1.0, 200, 9734 + y + count * 100, pClock, 0.25, 6) * pRadius);
        circlefill(dest, x + xOff, y + yOff, blobSize, getpixel (tempBitmap, pRadius + xOff, pRadius + yOff));
    }
    drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
    setUpdateArea(x - pRadius - blobSize, y - pRadius - blobSize, (pRadius + blobSize) * 2, (pRadius + blobSize) * 2);
    requireUpdate();

    destroy_bitmap(tempBitmap);
}

/*
 *Old version
void TELEPORT::draw(BITMAP *dest)
{
    BITMAP *tempBitmap;
    int blobSize = 8;
    int pClock = clock;
    if (pClock < 0)
    {
        pClock = 0;
        blobSize = (startClock / 2) / - clock;
    }

    tempBitmap = create_bitmap(radius * 2, radius * 2);
    blit(_env->db, tempBitmap, (int) remote->x - radius, (int) remote->y - radius, 0, 0, radius * 2, radius * 2);

    if (object)
        remote->draw(dest);

    drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
    set_trans_blender(0, 0, 0, 255 - (int)((pClock / startClock) * 255));
    for (int count = (radius * radius * 4 / (8 * 8)); count > pClock; count--)
    {
        int xOff = (int) (perlin2DPoint (1.0, 200, 1278 + (int)x + count * 100, clock, 0.25, 6) * (radius));
        int yOff = (int) (perlin2DPoint (1.0, 200, 9734 + (int)y + count * 100, clock, 0.25, 6) * (radius));
        circlefill(dest, (int) x + xOff, (int) y + yOff, blobSize, getpixel (tempBitmap, radius + xOff, radius + yOff));
    }
    drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
    setUpdateArea((int) x - radius - blobSize, (int) y - radius - blobSize, (radius + blobSize) * 2, (radius + blobSize) * 2);
    requireUpdate();

    destroy_bitmap(tempBitmap);
}
*/

int TELEPORT::isSubClass(int classNum)
{
    if (classNum == TELEPORT_CLASS)
        return TRUE;
    else
        return FALSE;
    //return (PHYSICAL_OBJECT::isSubClass(classNum));
}
