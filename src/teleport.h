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


#include "main.h"
#include "virtobj.h"

class TELEPORT: public VIRTUAL_OBJECT
{
public:
    int radius;
    int clock;
    int startClock;
    int peaked;
    int dispersing;
    VIRTUAL_OBJECT *object;
    TELEPORT *remote;

    virtual ~TELEPORT();
    TELEPORT(GLOBALDATA *global, ENVIRONMENT *env, VIRTUAL_OBJECT *targetObj, int destinationX, int destinationY, int objRadius, int duration);
    TELEPORT(GLOBALDATA *global, ENVIRONMENT *env, TELEPORT *remoteEnd, int destX, int destY);
    void initialise();
    void draw(BITMAP *dest);
    int applyPhysics();
    int isSubClass(int classNum);
    inline virtual int getClass()
    {
        return TELEPORT_CLASS;
    }
    inline virtual void setEnvironment(ENVIRONMENT *env)
    {
        if (!_env || (_env != env))
        {
            _env = env;
            _index = _env->addObject(this);
        }
    }
};

#endif
