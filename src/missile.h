#ifndef MISSILE_DEFINE
#define MISSILE_DEFINE

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

#define MAX_MISSLE_AGE 20
#define MAX_METEOR_AGE 5

#define TIGGER_HEIGHT 300

#define SDI_DISTANCE 100

class MISSILE: public PHYSICAL_OBJECT
{
private:
    // New values for growing napalm jelly:
    int iGrowRadius;
    int bIsGrowing;

public:
    int countdown;
    int expSize;
    int etime;
    int damage;
    int eframes;
    int picpoint;
    int type;
    int sound;
    int funky_colour;
    WEAPON *weap;

    virtual ~MISSILE();
    MISSILE(GLOBALDATA *global, ENVIRONMENT *env, double xpos, double ypos, double xvel, double yvel, int weaponType);
    void draw(BITMAP *dest);
    int triggerTest();
    void trigger();
    // void explode();
    int applyPhysics();
    void initialise();
    int isSubClass(int classNum);
    void setEnvironment(ENVIRONMENT *env);

    inline virtual int getClass()
    {
        return MISSILE_CLASS;
    }

    int Height_Above_Ground();
    TANK *Check_SDI(GLOBALDATA *global);       // see if missile should be shot down
};

#endif
