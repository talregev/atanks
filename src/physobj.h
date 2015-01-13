#ifndef PHYSOBJ_DEFINE
#define PHYSOBJ_DEFINE

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
#include "virtobj.h"
#include "globaldata.h"
//#include "environment.h"

class PHYSICAL_OBJECT: public VIRTUAL_OBJECT
{
public:
    int noimpact;
    int hitSomething;
    double mass;
    double drag;
    int spin;

    /* --- constructor --- */
    PHYSICAL_OBJECT() : VIRTUAL_OBJECT(),hitSomething(0) { }

    /* --- pure virtual methods --- */
    virtual int isSubClass(int classNum)_PURE;
    virtual int getClass()_PURE;

    /* --- non-virtual methods --- */

    /* --- inline methods --- */
    int applyPhysics();

    int checkPixelsBetweenPrevAndNow();

    void initialise();

    void draw(BITMAP *dest);
};

#endif
