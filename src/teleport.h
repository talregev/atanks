#ifndef TELEPORT_DEFINE
#define TELEPORT_DEFINE

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

#include "globaltypes.h"
#include "virtobj.h"

class TELEPORT: public VIRTUAL_OBJECT
{
  public:

	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */
	// Source constructor
	TELEPORT (VIRTUAL_OBJECT *targetObj,
	          int32_t destinationX, int32_t destinationY,
	          int32_t objRadius, int32_t duration, int32_t type);
	virtual ~TELEPORT ();


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void    applyPhysics ();
	void    draw ();

    eClasses getClass() { return CLASS_TELEPORT; }


private:

	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	// Target constructor
	TELEPORT (TELEPORT *remoteEnd, int32_t destX, int32_t destY);


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	int32_t         clock      = 0;
	VIRTUAL_OBJECT* object     = nullptr;
	int32_t         radius     = 0;
	TELEPORT*       remote     = nullptr;
	int32_t         startClock = 0;
};

#endif
