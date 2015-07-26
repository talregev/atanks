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
#include "tank.h"
#include "sound.h"

#ifdef NETWORK
#  include "player.h"
#endif

TELEPORT::~TELEPORT ()
{
	requireUpdate();
	update();
	if (dim_cur.w > 0)
		global.make_bgupdate (dim_cur.x, dim_cur.y, dim_cur.w, dim_cur.h);
	if (dim_old.w > 0)
		global.make_bgupdate (dim_old.x, dim_old.y, dim_old.w, dim_old.h);

	if (remote) {
		remote->destroy = true;
		remote->remote  = nullptr;
	}

	object = nullptr;
	remote = nullptr;

	// Take out of the chain:
	global.removeObject(this);
}

TELEPORT::TELEPORT (VIRTUAL_OBJECT *targetObj,
                    int32_t destinationX, int32_t destinationY,
                    int32_t objRadius, int32_t duration, int32_t type):
	VIRTUAL_OBJECT(),
	clock(duration),
	object(targetObj),
	radius(objRadius),
	startClock(duration)
{

	if (object) {
		x = object->x;
		y = object->y;
	}

	// Ensure the destination is not occupied by another tank:
	bool need_check = (type != ITEM_SWAPPER);
	while (need_check) {
		TANK* lt = nullptr;
		need_check = false;

		global.getHeadOfClass(CLASS_TANK, &lt);
		while ( lt ) {
			if ( (std::abs(lt->x - destinationX) < objRadius)
			  && (lt->y > destinationY)
			  && ((lt->y - destinationY) < objRadius) ) {
				need_check = true;

				// Maybe move left
				if ( ( (destinationX >  (objRadius * 2))
				    && (destinationX <= lt->x) )
				  || (destinationX >= (env.screenWidth - (objRadius * 2))) )
					destinationX -= std::abs(lt->x - destinationX);
				// Or move right
				else if (destinationX < (env.screenWidth - (objRadius * 2)))
					destinationX += std::abs(lt->x - destinationX);

				// Maybe move up
				if ( ( (destinationY >  (MENUHEIGHT + (objRadius * 2)) )
				    && (destinationY <= lt->y) )
				  || (destinationY >= (env.screenHeight - (objRadius * 2))) )
					destinationY -= std::abs(lt->y - destinationY);
				// Or move down
				else if (destinationY < (env.screenHeight - (objRadius * 2)))
					destinationY += std::abs(lt->y - destinationY);
			}


			lt->getNext(&lt);
		}
	} // end of needing to check the destination

	try {
		remote = new TELEPORT (this, destinationX, destinationY);
	} catch(std::bad_alloc &e) {
		std::cerr << "Error creating TELEPORT: " << e.what() << std::endl;
		perror ( "teleport.cc: Failed allocating memory for remote in TELEPORT::TELEPORT");
	}

	play_fire_sound(ITEM_TELEPORT + WEAPONS, x, 255, 1000);

#ifdef NETWORK
	// this seems to be the teleport we usually use
	int   playerindex = 0;
	bool  found       = false;
	TANK* the_tank    = static_cast<TANK*>(targetObj);

	// match the player with the tank
	while ( (playerindex < env.numGamePlayers) && (! found) ) {
		if ( (env.players[playerindex]->tank)
		  && (env.players[playerindex]->tank == the_tank) )
			found = true;
		else
			++playerindex;
	}

	if (found) {
		char  buffer[64]  = { 0x0 };
		snprintf(buffer, 63, "TELEPORT %d %d %d",
				playerindex, destinationX, destinationY);
		env.sendToClients(buffer);
	}
#endif // NETWORK

	// Add to the chain:
	global.addObject(this);
}




TELEPORT::TELEPORT (TELEPORT *remoteEnd, int32_t destX, int32_t destY) :
	VIRTUAL_OBJECT(),
	remote(remoteEnd)
{
	this->x = destX;
	this->y = destY;
	if (remote) {
		clock = remoteEnd->startClock;
		radius = remoteEnd->radius;
		startClock = remoteEnd->startClock;
	}

	// Add to the chain:
	global.addObject(this);
}

void TELEPORT::applyPhysics ()
{
	if (object) {
		if (!clock) {
			object->x = remote->x;
			object->y = remote->y;
			remote->object = object;
			object = nullptr;
			remote->clock--;
		}
	} else
		clock = remote->clock;

	if (clock-- < -startClock / 2)
		destroy = true;
}


void TELEPORT::draw ()
{
	if (!remote)
		return;

	double  pClock     = clock;
	int32_t blobSize   = 8;
	int32_t pRadius    = radius;
	int32_t maxblobs   = 1;

	// When the teleporting finishes, the blobs enlarge and disperse
	// using this then growing factor:
	if (pClock < 1.0)
		pClock = 1.0 + (1.0 - (pClock * 2.0));

	int32_t transMod = 255 - (pClock / startClock * 255);
	if (transMod > 255) transMod = 255;
	else if (transMod < 0) transMod = 0;

	blobSize -= round(8 / (startClock / pClock)) + 1;
	pRadius  -= round(radius / (startClock / pClock)) + 1;
	maxblobs += pRadius * 4;

	BITMAP* tempBitmap = create_bitmap (radius * 2, radius * 2);
	blit (global.canvas, tempBitmap,
		remote->x - radius, remote->y - radius, 0, 0,
		radius * 2, radius * 2);

	if (object && remote)
		remote->draw();

	drawing_mode (DRAW_MODE_TRANS, NULL, 0, 0);
	set_trans_blender (0, 0, 0, transMod);

	for (int32_t i = round(maxblobs + pClock); i > pClock; --i) {
		int32_t xOff = perlin2DPoint (1.0, 200, 1278 + x + (i * 100), pClock, 0.25, 6) * pRadius;
		int32_t yOff = perlin2DPoint (1.0, 200, 9734 + y + (i * 100), pClock, 0.25, 6) * pRadius;
		int32_t t_col = getpixel(tempBitmap, pRadius + xOff, pRadius + yOff);
		circlefill (global.canvas, x + xOff, y + yOff, blobSize, t_col);
	}

	drawing_mode (DRAW_MODE_SOLID, NULL, 0, 0);

	setUpdateArea (x - pRadius - blobSize, y - pRadius - blobSize,
	               (pRadius + blobSize) * 2, (pRadius + blobSize) * 2);
	requireUpdate ();

	destroy_bitmap (tempBitmap);
}
