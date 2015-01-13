#ifndef TANK_DEFINE
#define TANK_DEFINE

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

#define TELETIME 120

#define DIR_RIGHT 1
#define DIR_LEFT 2

#define VIOLENT_CHANCE 6


class PLAYER;
class EXPLOSION;
class FLOATTEXT;
class TANK: public PHYSICAL_OBJECT
{
protected:
    int _targetX,_targetY;
    int _prevTargetX,_prevTargetY;
    TANK *_target;

public:
    int bestPower, bestAngle;
    int smallestOvershoot;
    int t, a, p, cw, l, sh, sht, fs, ds, para, pen;
    int maxLife;    // amount awarded at beginning of round
    double damage;
    int repair_rate;
    int repulsion;
    int shieldColor, shieldThickness;
    int flashdamage, hit;
    int timer;
    int teleTimer;
    int teleXDest, teleYDest;
    double shPhase, delta_phase;
    PLAYER *creditTo;
    FLOATTEXT *healthText, *shieldText; //, *damageText;
    FLOATTEXT *nameText;
    int use_tank_bitmap, use_turret_bitmap;
    double turret_x, turret_y;
    int delay_fall;    // time the tank will hover
    int fire_another_shot;
    int shots_fired;

    ~TANK();
    TANK(GLOBALDATA *global, ENVIRONMENT *env);
    void initialise();
    void Destroy();
    int get_heaviest_shield();
    void boost_up_shield();
    void reactivate_shield();
    void drawTank(BITMAP *dest, int healthOffset);
    void printHealth(int offset);
    void update();
    void requireUpdate();
    void explode();
    void simActivateCurrentSelection();
    void activateCurrentSelection();
    int isSubClass(int classNum);
    inline virtual int getClass()
    {
        return TANK_CLASS;
    }
    virtual void setEnvironment(ENVIRONMENT *env);
    void applyDamage();
    int applyPhysics();
    void repulse(double xpos, double ypos, double *xaccel, double *yaccel, int aWeaponType);
    int howBuried();
    int shootClearance(int targetAngle, int minimumClearance = MAX_OVERSHOOT);
    void newRound();
    void framelyAccounting();
    int tank_on_tank(GLOBALDATA *gd);    //is this tank on top of another?
    int Get_Repair_Rate();    // how many units a tank will repair itself per round
    int Move_Tank(int direction);
    void Give_Credit(GLOBALDATA *global);    // give credit to killer
};

#endif
