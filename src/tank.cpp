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
#include "floattext.h"
#include "explosion.h"
#include "teleport.h"
#include "missile.h"
#include "player.h"
#include "beam.h"
#include "tank.h"


/*
The deconstructor should be split into two pieces. One function for
destroying a tank and another strictly for cleaning up a tank's memory
whether it was destoryed or not.
-- Jesse
*/
TANK::~TANK()
{
/*
#ifdef NETWORK
    if (player)    // we should always have a player, but better safe than sorry
    {
        int player_index = 0;
        bool found = false;
        char buffer[64];
        while ((player_index < _global->numPlayers) && !found)
        {
            // get the player index
            if ((_global->players[player_index]->tank) && (_global->players[player_index]->tank == this))
                found = true;
            else
                player_index++;
        }
        if (found)    // we should have found a match and now we send it to all clients
        {
            sprintf(buffer, "REMOVETANK %d", player_index);
            _global->Send_To_Clients(buffer);
        }
    }
#endif
    if (_global && player)
    {
        int random_item;
        if (_global->violent_death)
        {
            random_item = rand() % VIOLENT_CHANCE;
            if ((random_item > _global->violent_death) && (random_item <= VIOLENT_DEATH_HEAVY))
                random_item = (int) _global->violent_death;

            switch (random_item)
            {
            case VIOLENT_DEATH_LIGHT:
                player->ni[ITEM_VENGEANCE] += 1;
                break;
            case VIOLENT_DEATH_MEDIUM:
                player->ni[ITEM_DYING_WRATH] += 1;
                break;
            case VIOLENT_DEATH_HEAVY:
                player->ni[ITEM_FATAL_FURY] += 1;
                break;
            }    //end of switch
        }

        if (player->ni[ITEM_FATAL_FURY] > 0)
        {
            int numLaunch = (int) item[ITEM_FATAL_FURY].vals[SELFD_NUMBER];
            cw = (int)item[ITEM_FATAL_FURY].vals[SELFD_TYPE];
            player->ni[ITEM_FATAL_FURY]--;
            player->nm[cw] += numLaunch;
            for (int count = numLaunch; count > 0; count--)
            {
                a = rand() % 180 + 90;
                p = rand() % (MAX_POWER / 2);
                activateCurrentSelection();
            }
        }
        else if (player->ni[ITEM_DYING_WRATH] > 0)
        {
            int numLaunch = (int) item[ITEM_DYING_WRATH].vals[SELFD_NUMBER];
            cw = (int) item[ITEM_DYING_WRATH].vals[SELFD_TYPE];
            player->ni[ITEM_DYING_WRATH]--;
            player->nm[cw] += numLaunch;
            for (int count = numLaunch; count > 0; count--)
            {
                a = rand() % 180 + 90;
                p = rand() % (MAX_POWER / 2);
                activateCurrentSelection();
            }
        }
        else if (player->ni[ITEM_VENGEANCE] > 0)
        {
            int numLaunch = (int) item[ITEM_VENGEANCE].vals[SELFD_NUMBER];
            cw = (int) item[ITEM_VENGEANCE].vals[SELFD_TYPE];
            player->ni[ITEM_VENGEANCE]--;
            player->nm[cw] += numLaunch;
            for (int count = numLaunch; count > 0; count--)
            {
                a = rand() % 180 + 90;
                p = rand() % (MAX_POWER / 2);
                activateCurrentSelection();
            }
        }
    }
*/

    if (player)
    {
        player->tank = NULL;
        player = NULL;
    }
    if (_global)
    {
        _global->numTanks--;
        if (_global->currTank == this)
            _global->currTank = NULL;
    }

    if (shieldText)
        delete (shieldText);
    if (healthText)
        delete (healthText);
    if (nameText)
        delete (nameText);

    if (_env)
        _env->removeObject(this);

    shieldText = NULL;
    healthText = NULL;
    nameText = NULL;
    _env = NULL;
    _global = NULL;
    creditTo = NULL;
    _target = NULL;
}

TANK::TANK(GLOBALDATA *global, ENVIRONMENT *env) : PHYSICAL_OBJECT(),_target(NULL),creditTo(NULL),
           healthText(NULL),shieldText(NULL),nameText(NULL)
{
    setEnvironment(env);
    _global = global;
    // Ask for memory
    healthText = new FLOATTEXT(global, env, NULL, 0, 0, WHITE, CENTRE);
    if (!healthText)
    {
        perror("tank.cpp: Failed allocating memory for healthText in TANK::TANK");
        // exit (1);
    }

    shieldText = new FLOATTEXT(global, env, NULL, 0, 0, makecol (200, 200, 255), CENTRE);
    if (!shieldText)
    {
        perror("tank.cpp: Failed allocating memory for shieldText in TANK::TANK");
        // exit (1);
    }

    if (global->name_above_tank)
    {
        nameText = new FLOATTEXT(global, env, NULL, 0, 0, WHITE, CENTRE);
        if (!nameText)
        {
            perror("tank.cpp: Failed allocating memory for nameText in TANK::TANK");
            // exit(1);
        }
    }

    // Other initial pointers:
    _align = LEFT;
    _global->numTanks++;

    player = NULL;
}

void TANK::initialise()
{
    PHYSICAL_OBJECT::initialise();
    drag = 0.5;
    mass = 3000;
    repulsion = 0;
    shieldColor = 0;
    shieldThickness = 0;
    t = 0;
    sh = 0;
    _targetX = -1;
    _targetY = -1;
    newRound ();
}

void TANK::newRound()
{
    char buf[10];

    cw = 0;
    damage = 0;
    pen = 0;
    para = 0;
    creditTo = NULL;
    p = MAX_POWER / 2;
    a = (rand () % 180) + 90;
    if (sh > 0 && sht > 0)
        if (player)
            player->ni[sht]++;
    sh = 0;
    repulsion = 0;
    // shPhase = rand() % 360;
    shPhase = 0;
    delta_phase = 0.1;
    sht = 0;
    l = 100;
    repair_rate = 0;
    if (player)
    {
        double tmpL = 0;
        tmpL += (int) (player->ni[ITEM_ARMOUR] * item[ITEM_ARMOUR].vals[0]);
        tmpL += (int) (player->ni[ITEM_PLASTEEL] * item[ITEM_PLASTEEL].vals[0]);
        repair_rate = Get_Repair_Rate();
        if (tmpL > 0)
            l += (int) pow(tmpL, 0.6);

        if (healthText)
            healthText->set_color(player->color);
        if (nameText)
        {
            nameText->set_text(player->getName());
            nameText->set_color(player->color);
        }
    }
    maxLife = l;
    ds = 0;
    fs = 0;
    sprintf(buf, "%d", l);
    healthText->set_text(buf);
    // delay_fall = GRAVITY_DELAY;
    delay_fall = static_cast<int>(std::abs(_env->landSlideDelay * 100));
    fire_another_shot = 0;
    shots_fired = 0;

}

void TANK::Destroy()
{
#ifdef NETWORK
    if (player)    // we should always have a player, but better safe than sorry
    {
        int player_index = 0;
        bool found = false;

        while ((player_index < _global->numPlayers) && !found)
        {
            // get the player index
            if ((_global->players[player_index]->tank) && (_global->players[player_index]->tank == this))
                found = true;
            else
                player_index++;
        }
        if (found)    // we should have found a match and now we send it to all clients
        {
            char buffer[64];

            sprintf(buffer, "REMOVETANK %d", player_index);
            _global->Send_To_Clients(buffer);
        }
    }
#endif

    if (_global && player)
    {
        if (_global->violent_death)
        {
            int random_item;
            random_item = rand() % VIOLENT_CHANCE;
            if ((random_item > _global->violent_death) && (random_item <= VIOLENT_DEATH_HEAVY))
                random_item = (int) _global->violent_death;

            switch (random_item)
            {
            case VIOLENT_DEATH_LIGHT:
                player->ni[ITEM_VENGEANCE] += 1;
                break;
            case VIOLENT_DEATH_MEDIUM:
                player->ni[ITEM_DYING_WRATH] += 1;
                break;
            case VIOLENT_DEATH_HEAVY:
                player->ni[ITEM_FATAL_FURY] += 1;
                break;
            }    //end of switch
        }

        if (player->ni[ITEM_FATAL_FURY] > 0)
        {
            int numLaunch = (int) item[ITEM_FATAL_FURY].vals[SELFD_NUMBER];
            cw = (int) item[ITEM_FATAL_FURY].vals[SELFD_TYPE];
            player->ni[ITEM_FATAL_FURY]--;
            player->nm[cw] += numLaunch;
            for (int count = numLaunch; count > 0; count--)
            {
                a = rand() % 180 + 90;
                p = rand() % (MAX_POWER / 2);
                activateCurrentSelection();
            }
        }
        else if (player->ni[ITEM_DYING_WRATH] > 0)
        {
            int numLaunch = (int) item[ITEM_DYING_WRATH].vals[SELFD_NUMBER];
            cw = (int) item[ITEM_DYING_WRATH].vals[SELFD_TYPE];
            player->ni[ITEM_DYING_WRATH]--;
            player->nm[cw] += numLaunch;
            for (int count = numLaunch; count > 0; count--)
            {
                a = rand() % 180 + 90;
                p = rand() % (MAX_POWER / 2);
                activateCurrentSelection();
            }
        }
        else if (player->ni[ITEM_VENGEANCE] > 0)
        {
            int numLaunch = (int) item[ITEM_VENGEANCE].vals[SELFD_NUMBER];
            cw = (int) item[ITEM_VENGEANCE].vals[SELFD_TYPE];
            player->ni[ITEM_VENGEANCE]--;
            player->nm[cw] += numLaunch;
            for (int count = numLaunch; count > 0; count--)
            {
                a = rand() % 180 + 90;
                p = rand() % (MAX_POWER / 2);
                activateCurrentSelection();
            }
        }
    }
}

void TANK::update()
{
    VIRTUAL_OBJECT::update();
}

void TANK::requireUpdate()
{
    VIRTUAL_OBJECT::requireUpdate();
}

void TANK::applyDamage()
{
    char buf[10];
    bool killed = false;

    if (damage > sh + l)
    {
        damage = sh + l;
        killed = true;
        player->killed++;
    }
    sh -= (int) damage;
    if (creditTo)
    {
        char *temp_text;

        if (player != creditTo)    //enemy hit ++
        {
            if (killed)
                creditTo->kills++;
            creditTo->money += (int) (damage * _global->scoreHitUnit);
            if (creditTo->tank)
            {
                char the_money[64];
                sprintf(the_money, "$%s", Add_Comma((int) (damage * _global->scoreHitUnit)));
                // show how much the shooter gets
                FLOATTEXT *moneyText = new FLOATTEXT(_global, _env, the_money,
                                                     (int) creditTo->tank->x, (int) creditTo->tank->y - 30,
                                                     makecol(0, 255, 0), CENTRE);
                if (moneyText)
                {
                    // moneyText->xv = 0;
                    // moneyText->yv = -0.5;
                    moneyText->set_speed(0.0, -0.5);
                    moneyText->maxAge = 200;
                }
                
            }
            // this tank is destroyed, the attacker gloats
            if ((killed) && (!creditTo->gloating))
            {
                // avoid trying to print victory message over a dead tank
                if (creditTo->tank)
                {
                    temp_text = creditTo->selectGloatPhrase();
                    FLOATTEXT *gloatText = new FLOATTEXT(_global, _env,
                                                      // creditTo->selectGloatPhrase( (double) damage / maxLife),
                                                         temp_text,
                                                         (int) creditTo->tank->x, (int) creditTo->tank->y - 30,
                                                         creditTo->color, CENTRE);
                    if (gloatText)
                    {
                        // gloatText->xv = 0;
                        // gloatText->yv = -0.4;
                        gloatText->set_speed(0.0, -0.4);
                        gloatText->maxAge = 300;
                    }
                    else
                        perror("tank.cpp: Failed allocating memory for gloatText in applyDamage.");
                    creditTo->gloating = true;
                }
            }

            if ((int) player->type != HUMAN_PLAYER)
            {
                if (player->revenge == creditTo)
                    player->annoyanceFactor += damage;
                else
                    player->annoyanceFactor = damage;
                if ((player->annoyanceFactor > (player->vengeanceThreshold * maxLife))
                    &&((rand() % 100) <= player->vengeful))
                {
                    player->revenge = creditTo;
                    temp_text = player->selectRevengePhrase();
                    FLOATTEXT *revengeText = new FLOATTEXT(_global, _env,
                                                        // player->selectRevengePhrase ((double)damage / maxLife),
                                                           temp_text, (int) x, (int) y - 30, player->color, CENTRE);
                    if (revengeText)
                    {
                        // revengeText->xv = 0;
                        // revengeText->yv = -0.4;
                        revengeText->set_speed(0.0, -0.4);
                        revengeText->maxAge = 300;
                    }
                    else
                        perror("tank.cpp: Failed allocating memory for revengeText in applyDamage");
                }
            }
        }
        else    //self hit --
        {
            if ((creditTo->money - (damage * _global->scoreSelfHit)) < 0)
                creditTo->money = 0;
            else
                creditTo->money -= (int) (damage * _global->scoreSelfHit);

            if (damage >= (l + sh))
            {
                temp_text = player->selectSuicidePhrase();
                FLOATTEXT *suicideText = new FLOATTEXT(_global, _env, player->selectSuicidePhrase(),
                                                       (int) x, (int) y - 30, player->color, CENTRE);
                if (suicideText)
                {
                    // suicideText->xv = 0;
                    // suicideText->yv = -0.4;
                    suicideText->set_speed(0.0, -0.4);
                    suicideText->maxAge = 300;
                }
                else
                    perror("tank.cpp: Failed allocating memory for suicideText in applyDamage.");
            }
        }
    }

    if (sh < 0)
        l += sh;
    if (l < 0)
        l = 0;
    if (sh <= 0)
    {
        repulsion = 0;
        shieldColor = 0;
        shieldThickness = 0;
        sh = 0;
    }

    if ((int) damage > 0)
    {
        flashdamage = 1;
        sprintf(buf, "%d", (int) damage);
        FLOATTEXT *damageText = new FLOATTEXT(_global, _env, NULL, (int) x, (int) y, makecol(255, 0, 0), CENTRE);
        if (damageText)
        {
            damageText->set_text(buf);
            // damageText->xv = 0;
            // damageText->yv = -0.2;
            damageText->set_speed(0.0, -0.2);
            damageText->maxAge = 300;
            // damageText->sway = NORMAL_SWAY;
        }
        else
            perror("tank.cpp: Failed allocating memory for damageText in TANK::TANK");
    }
    if (sh > 0)
    {
        sprintf(buf, "%d", sh);
        shieldText->set_text(buf);
    }
    else
        shieldText->set_text(NULL);

    sprintf(buf, "%d", l);
    healthText->set_text(buf);
}

void TANK::framelyAccounting()
{
/*
    if (shPhase < 0)
        shPhase = rand() % 360;
    shPhase += (int) (item[sht].vals[SHIELD_ENERGY] / sh) + 10;
    while (shPhase >= 360)
        shPhase -= 360;
*/
    shPhase = shPhase + delta_phase;
    if ( ( shPhase > 5) || (shPhase < -2) )
        delta_phase = -delta_phase;
}

int TANK::applyPhysics()
{
    int stable = 0;

    // if we are buried, rockets shouldn't work
    if (howBuried())
    {
        xv = 0;
        if (yv < 0)
            yv = 0;
    }

    // make sure the tank does not leave the screen when flying
    if ((yv < 0) && (y < TANKHEIGHT))
        yv = 0;

    if (yv < 0)
        x += xv * 4;

    if (!flashdamage)
    {
        int landed_on_tank;

        if (x + xv < 1 || x + xv > (_global->screenWidth-1))
            xv = -xv;    //bounce on the border
        int pixelCol = getpixel(_env->terrain, (int) x, (int) y + (TANKHEIGHT - TANKSAG));

        // check to see if we have landed on another tank -- Jesse
        landed_on_tank = tank_on_tank( _global);

        // we are falling and have hit the bottom of the screen or fallen onto dirt or on a tank
        if ( (l > 0) && (yv > 0) && ((y >= _global->screenHeight - TANKHEIGHT)
                                     || (pixelCol != PINK) || (landed_on_tank)) )
        {
            //count damage and add money
            damage = (int) (yv * 10);
            if (damage >= 10)
                damage -= 10;
            else damage = 0;
            creditTo = NULL;
            yv = 0;
            xv = 0;
            // if we passed the bottom, then stop on bottom
            if (y > _global->screenHeight - TANKHEIGHT)
                y = _global->screenHeight - TANKHEIGHT;

            // delay_fall = GRAVITY_DELAY;
            delay_fall = (int) _env->landSlideDelay * 100;

        }

        // the tank is falling
        else if ((y < _global->screenHeight - TANKHEIGHT) && (pixelCol == PINK) && (l > 0) && !landed_on_tank
                 && (_env->landSlideType > LANDSLIDE_NONE))
        {
            delay_fall--;
            if ((delay_fall > 0) && (_env->landSlideType == LANDSLIDE_CARTOON))
                return stable;

#ifdef OLD_GAMELOOP
            if (para && para < 3 && !_env->pclock)
                para++;
#else
            if (para && (para < 3))
                para++;
#endif
            if (!para)
            {
                yv += _env->gravity * (100.0 / _global->frames_per_second);
                y += yv;
            }
            else
            {
                double accel = (_env->wind - xv) / mass * (drag + 0.35) * _env->viscosity;
                xv += accel;
                yv += _env->gravity * (100.0 / _global->frames_per_second);
                if (yv > 0.5)
                    yv = 0.5;
                x += xv;
                y += yv;
            }
            // falling, deploy parachute
            if (!para)
            {
                if ((player->ni[ITEM_PARACHUTE]) && (yv >= 1.0))
                {
                    _env->pclock = 1;
                    para = 1;
                    player->ni[ITEM_PARACHUTE]--;
                }

            }
            requireUpdate();
        }
        else
        {
            stable = 1;
            if (damage && !pen)
            {
                applyDamage();
                pen = 1;
            }
            para = 0;
        }
    }
    else
        flashdamage++;

    return stable;
}

void TANK::explode()
{
    EXPLOSION *explosion;

    if ((int) player->type != HUMAN_PLAYER)
    {
        char *temp_text;

        player->revenge = creditTo;
        temp_text = player->selectRevengePhrase();
        FLOATTEXT *revengeText = new FLOATTEXT(_global, _env, temp_text, (int)x, (int)y - 30, player->color, CENTRE);
        if (revengeText)
        {
            // revengeText->xv = 0;
            // revengeText->yv = -0.4;
            revengeText->set_speed(0.0, -0.4);
            revengeText->maxAge = 300;
        }
        else
            perror("tank.cpp: Failed allocating memory for revengeText in TANK::explode");
    }
    explosion = new EXPLOSION(_global, _env, x, y, 1);
    if (explosion)
    {
        explosion->player = player;
        explosion->bIsWeaponExplosion = false;
    }
    else
        perror("tank.cpp: Failed allocating memory for explosion in TANK::explode");


    destroy = TRUE;
    play_sample((SAMPLE *)_global->sounds[WEAPONSOUNDS], _env->scaleVolume(255), 128, 1000, 0);
}

void TANK::repulse(double xpos, double ypos, double *xaccel, double *yaccel, int aWeaponType)
{
    if (repulsion != 0)
    {
        double xdist = xpos - x;
        double ydist = -1.0 * fabs(ypos - y);

        if ((xdist < 0.1) && (xdist > -0.1))
            xdist = 0.1;
        if ((ydist < 0.1) && (ydist > -0.1))
            ydist = -0.1; // Assume missile comes from above

        if ((aWeaponType >= BURROWER) && (aWeaponType <= PENETRATOR) && (ypos > y))
            ydist *= -1.0; // they normally come from below!

        double distance2 = (xdist * xdist) + (ydist * ydist);
        double distance = sqrt (distance2);

        if (distance < (60.0 + sqrt((double) repulsion)))
        {
            *xaccel = repulsion * (xdist / distance) / distance2;
            *yaccel = repulsion * (ydist / distance) / distance2;
        }
    }
}

void TANK::printHealth(int offset)
{
    int textpos = -5;

    shieldText->set_pos((int) x, (int) y - TANKHEIGHT + textpos + offset);
    if (sh > 0)
        textpos -= 10;
    healthText->set_pos((int) x, (int) y - TANKHEIGHT + textpos + offset);

    // display player name
    if (nameText)
    {
        textpos -= 10;
        nameText->set_pos((int) x, (int) y - TANKHEIGHT + textpos + offset);
    }
}

void TANK::drawTank(BITMAP *dest, int healthOffset)
{
    int turretAngle;

    // check for foggy weather
    if ((_env->fog) && (_global->currTank != this))
    {
        addUpdateArea((int) (x - TANKWIDTH) - 3, (int) y - 25, 35, 46);
        requireUpdate();
        return;
    }

    // get bitmap for tank
    if (player)
    {
        switch ((int) player->tank_bitmap)
        {
        case CLASSIC_TANK:
            use_tank_bitmap = 8;
            use_turret_bitmap = 1;
            turret_x = x;
            turret_y = y + (TANKHEIGHT / 2);
            break;
        case BIGGREY_TANK:
            use_tank_bitmap = 9;
            use_turret_bitmap = 2;
            turret_y = y;
            turret_x = x;
            break;
        case T34_TANK:
            use_tank_bitmap = 10;
            use_turret_bitmap = 3;
            turret_y = y;
            turret_x = x;
            break;
        case HEAVY_TANK:
            use_tank_bitmap = 11;
            use_turret_bitmap = 4;
            turret_y = y;
            turret_x = x;
            break;
        case FUTURE_TANK:
            use_tank_bitmap = 12;
            use_turret_bitmap = 5;
            turret_y = y;
            turret_x = x;
            break;
        case UFO_TANK:
            use_tank_bitmap = 13;
            use_turret_bitmap = 6;
            turret_y = y;
            turret_x = x;
            break;
        case SPIDER_TANK:
            use_tank_bitmap = 14;
            use_turret_bitmap = 7;
            turret_y = y;
            turret_x = x;
            break;
        case BIGFOOT_TANK:
            use_tank_bitmap = 15;
            use_turret_bitmap = 8;
            turret_y = y;
            turret_x = x;
            break;
        case MINI_TANK:
            use_tank_bitmap = 16;
            use_turret_bitmap = 9;
            turret_y = y;
            turret_x = x;
            break;
        default:
            use_tank_bitmap = 0;
            use_turret_bitmap = 0;
            turret_x = x;
            turret_y = y;
            break;
        }
    }

    rectfill(dest, (int) x - (TANKWIDTH - 1), (int) (y + TANKHEIGHT) - 2,
             (int) (x + TANKWIDTH) - 2, (int) (y + TANKHEIGHT), this->player->color);
    // draw_sprite(dest, (BITMAP *) _global->gfxData.T[use_tank_bitmap].dat, (int) x - TANKWIDTH, (int) y);

/*
    Drawing shields this way seems to cause a crash on multi-cpu
      systems. Taking this out and creating new shield
      drawing routine below. -- Jesse

    if (sh > 0)
    {
        int phaseValue = 1;

        drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
        _env->current_drawing_mode = DRAW_MODE_TRANS;
        set_trans_blender(0, 0, 0, (int) 50);
        // avoid sub-index
        if (shPhase < 0)
            shPhase = 0;
        if (sht >= ITEM_LGT_SHIELD && sht <= ITEM_HVY_SHIELD)
            phaseValue = (int) (_global->slope[(int) shPhase][0] * 3);
        else if (sht >= ITEM_LGT_REPULSOR_SHIELD && sht <= ITEM_HVY_REPULSOR_SHIELD)
            phaseValue = (int) (shPhase / 360 * 6);

        ellipsefill(dest, (int) x, (int) y, TANKWIDTH + 6 + phaseValue, TANKHEIGHT - 1, shieldColor);
        set_trans_blender(0, 0, 0,
                          (int) (50 + (((float) sh / (float) (item[sht]).vals[SHIELD_ENERGY]) * (float) 150)));
        for (int thicknessCount = 0; thicknessCount < shieldThickness; thicknessCount++)
        {
            ellipse(dest, (int) x - (shieldThickness / 2) + thicknessCount, (int) y,
                    TANKWIDTH + 6 + phaseValue, TANKHEIGHT - 1, shieldColor);
        }
        drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
        _env->current_drawing_mode = DRAW_MODE_SOLID;
    }    // end of drawing shield
*/

    if (sh > 0)
    {
        int thickness = 2;
        int counter;
        int wobble = (int) shPhase;

        if ((sht == ITEM_LGT_SHIELD) || (sht == ITEM_LGT_REPULSOR_SHIELD))
            thickness = 1;
        else if ((sht == ITEM_HVY_REPULSOR_SHIELD) || (sht == ITEM_HVY_SHIELD))
            thickness = 3;

        if (!shieldColor)    // client may not have set colour
            shieldColor = makecol((int) item[sht].vals[SHIELD_RED], (int) item[sht].vals[SHIELD_GREEN],
                                  (int) item[sht].vals[SHIELD_BLUE]);
        for (counter = 0; counter < thickness; counter++)
            circle(dest, (int) x, (int) y, TANKHEIGHT + counter + wobble, shieldColor);
    }

    draw_sprite(dest, (BITMAP *) _global->tank[use_tank_bitmap], (int) x - TANKWIDTH, (int) y);
    turretAngle = (int) ((float) (90 - a) * ((float) 256 / (float) 360));
    rotate_sprite(dest, (BITMAP *) _global->tankgun[use_turret_bitmap],
                  (int) turret_x - GUNLENGTH, (int) turret_y - (GUNLENGTH - 2), itofix (turretAngle));


    // when using rockets, show flame
    if (yv < 0)
        rectfill(dest, (int) x - TANKWIDTH, (int) y + TANKHEIGHT, x + TANKWIDTH, y + TANKHEIGHT + 10, makecol(250, 150, 0));

    if (sh > 0)
        setUpdateArea((int)x - TANKWIDTH - 15, (int)y - TANKHEIGHT, ((TANKWIDTH + 15) * 2) + 1, (TANKHEIGHT * 2) + 20);
    else
        setUpdateArea((int)x - GUNLENGTH - 1, (int)y - GUNLENGTH - 1, (GUNLENGTH * 2) + 2, TANKHEIGHT + GUNLENGTH + 20);
    if (para)
    {
        draw_sprite(dest, (BITMAP *) _global->tank[para], (int) (x - TANKWIDTH) - 3, (int) y - 25);
        addUpdateArea((int) (x - TANKWIDTH) - 3, (int) y - 25, 35, 66);
    }

    printHealth(healthOffset);
    requireUpdate();
}

int TANK::get_heaviest_shield()
{
    if (player->ni[ITEM_HVY_REPULSOR_SHIELD])
        return ITEM_HVY_REPULSOR_SHIELD;

    if (player->ni[ITEM_HVY_SHIELD])
        return ITEM_HVY_SHIELD;

    if (player->ni[ITEM_MED_REPULSOR_SHIELD])
        return ITEM_MED_REPULSOR_SHIELD;

    if (player->ni[ITEM_MED_SHIELD])
        return ITEM_MED_SHIELD;

    if (player->ni[ITEM_LGT_REPULSOR_SHIELD])
        return ITEM_LGT_REPULSOR_SHIELD;

    if (player->ni[ITEM_LGT_SHIELD])
        return ITEM_LGT_SHIELD;

    return ITEM_NO_SHIELD;
}

void TANK::simActivateCurrentSelection()
{
    char buf[16];

    if (_global->turntype != TURN_SIMUL)
    {
        activateCurrentSelection();
        if (fire_another_shot)
            fire_another_shot--;
    }
    else
        _env->stage = 1;

    // allow naturals to happen again
    _env->naturals_since_last_shot = 0;

    // apply repairs
    l += repair_rate;
    if (l > maxLife)
        l = maxLife;
    sprintf(buf, "%d", l);
    healthText->set_text(buf);

    // avoid having key presses read in next turn
    clear_keybuf();
}

void TANK::activateCurrentSelection()
{
    // avoid firing weapons on exit in Windows
    if ((_global->get_command() == GLOBAL_COMMAND_QUIT) || (_global->get_command() == GLOBAL_COMMAND_MENU))
        return;

    // remove status from top bar at next redraw
    if (_global->tank_status)
        _global->tank_status[0] = 0;

    _env->time_to_fall--;
    if (_env->time_to_fall < 0)
        // _env->time_to_fall = (rand() % MAX_GRAVITY_DELAY) + 1;
        _env->time_to_fall = (rand() % (int) _env->landSlideDelay) + 1;

    if (cw < WEAPONS)
    {
        player->changed_weapon = false;
        if (cw)
            player->nm[cw]--;

        _env->stage = 1;
        _env->am = weapon[cw].spread;
        _env->realm = _env->am;

        if (cw < BALLISTICS)
        {
            play_sample((SAMPLE *) _global->sounds[weapon[cw].sound], _env->scaleVolume(255), 128, 1000, 0);
            for (int z = 0; z < _env->am; z++)
            {
                MISSILE *newmis;
                double mxv,myv;
                int ca;

                ca = a + ((SPREAD * z) - (SPREAD * (_env->am - 1) / 2));
                double dPower = (double) p;
                if ((dPower < 200.0) && ((cw == RIOT_CHARGE) || (cw == RIOT_BLAST)))
                    dPower = 200.0;

                mxv = _global->slope[ca][0] * dPower * (100.0 / _global->frames_per_second) / 100.0;
                myv = _global->slope[ca][1] * dPower * (100.0 / _global->frames_per_second) / 100.0;

                newmis = new MISSILE(_global, _env,
                                     turret_x + (_global->slope[ca][0] * GUNLENGTH) /*- mxv*/,
                                     turret_y + (_global->slope[ca][1] * GUNLENGTH) /*- myv*/,
                                     mxv, myv, cw);
                if (newmis)
                {
                    newmis->physics = 0;
                    newmis->age = 0;
                    newmis->player = player;
                }
                else
                    perror("tank.cpp: Failed allocating memory for newmis in TANK::activateCurrentSelection");
                // set up volley
                if (!fire_another_shot)
                {
                    fire_another_shot = weapon[cw].delay * VOLLY_DELAY;
                    if (weapon[cw].delay)
                        shots_fired = shots_fired - (weapon[cw].delay - 1);
                }

                if (player->ni[ITEM_DIMPLEP])
                {
                    player->ni[ITEM_DIMPLEP]--;
                    newmis->drag *= item[ITEM_DIMPLEP].vals[0];
                }
                else if (player->ni[ITEM_SLICKP])
                {
                    player->ni[ITEM_SLICKP]--;
                    newmis->drag *= item[ITEM_SLICKP].vals[0];
                }
            }
        }
        else    // BEAM weapon
        {
            play_sample((SAMPLE *) _global->sounds[weapon[cw].sound], _env->scaleVolume(255), 128, 1000, 0);
            for (int z = 0; z < _env->am; z++)
            {
                BEAM *newbeam;
                int ca;

                ca = a + ((SPREAD * z) - (SPREAD * (_env->am - 1) / 2));

                newbeam = new BEAM(_global, _env, turret_x + (_global->slope[ca][0] * GUNLENGTH),
                                   turret_y + (_global->slope[ca][1] * GUNLENGTH), ca, cw);
                if (newbeam)
                {
                    newbeam->physics = 0;
                    newbeam->age = 0;
                    newbeam->player = player;
                }
                else
                    perror("tank.cpp: Failed allocating memory for newbeam in TANK::activateCurrentSelection");
            }

        }
    }
    else    // activate an item
    {
        int itemNum = cw - WEAPONS;
        if (itemNum < ITEM_VENGEANCE || itemNum > ITEM_FATAL_FURY)
            player->ni[itemNum]--;
        _env->stage = 1;
        if (itemNum == ITEM_TELEPORT)
        {
            int teleXDest = (rand() % (_global->screenWidth - TANKWIDTH * 2)) + TANKWIDTH;
            int teleYDest = (rand() % (_global->screenHeight - TANKHEIGHT * 2)) + TANKHEIGHT;
            TELEPORT *teleport;
            creditTo = player;
            teleport = new TELEPORT(_global, _env, this, teleXDest, teleYDest, TANKHEIGHT * 4 + GUNLENGTH, 120);
            if (!teleport)
            {
                perror("tank.cpp: Failed allocating memory for teleport in TANK::activateCurrentSelection");
                // exit (1);
            }
        }
        else if (itemNum == ITEM_SWAPPER)
        {
            int random_tank_number;
            TANK *other_tank;

            // pick a random tank (not us)
            random_tank_number = rand() % _global->numTanks;
            other_tank = _env->order[random_tank_number];
            while (!other_tank || (other_tank == this))
            {
                random_tank_number++;
                if (random_tank_number > _global->maxNumTanks)
                    random_tank_number = 0;
                other_tank = _env->order[random_tank_number];
            }
            creditTo = player;
            // create a teleport object for this tank
            new TELEPORT(_global, _env, this, (int) other_tank->x, (int) other_tank->y, TANKHEIGHT * 4 + GUNLENGTH, 120);
            // create a teleport object for the other tank
            new TELEPORT(_global, _env, other_tank, (int) x, (int) y, TANKHEIGHT * 4 + GUNLENGTH, 120);

        }
        else if (itemNum == ITEM_MASS_TELEPORT)
        {
            int XDest, YDest;

            for (int count = 0; count < _global->numPlayers; count++)
            {
                TANK *current_tank = _global->players[count]->tank;
                if (current_tank)
                {
                    XDest = (rand() % (_global->screenWidth - TANKWIDTH * 2)) + TANKWIDTH;
                    YDest = (rand() % (_global->screenHeight - TANKHEIGHT * 2)) + TANKHEIGHT;
                    creditTo = player;
                    new TELEPORT(_global, _env, current_tank, XDest, YDest, TANKHEIGHT * 4 + GUNLENGTH, 120);
                }
            }
        }

        else if (itemNum == ITEM_ROCKET)
        {
            yv = -10;
            y -= 10;
            if (a < 180)
                xv += 0.3;
            else if (a > 180)
                xv -= 0.3;

            applyPhysics();
        }

        else if (itemNum == ITEM_FAN)
        {
            // play wind sound
            play_sample((SAMPLE *) _global->sounds[2], _env->scaleVolume(255), 128, 1000, 0);
            if (a < 180)    // move wind to the right
                _env->wind += (p / 20);
            else     // wind to the left
                _env->wind -= (p / 20);

            // make sure wind is not too strong
            if (_env->wind < (-_env->windstrength / 2))
                _env->wind = -_env->windstrength / 2;
            else if (_env->wind > (_env->windstrength / 2))
                _env->wind = _env->windstrength / 2;

            _env->lastwind = _env->wind;

        }
        else if ((itemNum >= ITEM_VENGEANCE) && (itemNum <= ITEM_FATAL_FURY))
        {
            creditTo = player;
            damage = l + sh;
        }
    }

    // if we are out of this type of weapon
    // then switch to another
    if (!player->nm[cw])
    {
        cw = WEAPONS - 1;
        while ((cw > 0) && (!player->nm[cw]))
            cw--;
        player->changed_weapon = true;
    }

    shots_fired++;
    player->time_left_to_fire = _global->max_fire_time;

    // if not performing a volly and the player is AI then randomly select next weapon
    // else if ((player->type > HUMAN_PLAYER) && !fire_another_shot)
    //     cw = player->Select_Random_Weapon();

}

void TANK::boost_up_shield()
{
    int s = get_heaviest_shield();

    if ((s != ITEM_NO_SHIELD) && (player->ni[s] > 0))
    {
        player->ni[s]--;
        sh = (int)item[s].vals[SHIELD_ENERGY];
        repulsion = (int)item[s].vals[SHIELD_REPULSION];
        shieldColor = makecol((int) item[s].vals[SHIELD_RED], (int) item[s].vals[SHIELD_GREEN],
                              (int) item[s].vals[SHIELD_BLUE]);
        shieldThickness = (int) item[s].vals[SHIELD_THICKNESS];
        sht = s;
        ds = sht;
        player->last_shield_used = s;
    }
    if (sh)
    {
        char buf[10];

        sprintf (buf, "%d", sh);
        shieldText->set_text(buf);
    }
    else
        shieldText->set_text(NULL);
}

void TANK::reactivate_shield()
{
    if (!sh)    //if no shield remains, try to reload
        boost_up_shield();
}

int TANK::howBuried()
{
    int turrAngle;
    int buryCount = 0;

    for (turrAngle = 90; turrAngle < 270; turrAngle++)
    {
        if (getpixel(_env->terrain, (int)(x + (_global->slope[turrAngle][0] * GUNLENGTH)), (int)(y + (_global->slope[turrAngle][1] * GUNLENGTH))) != PINK)
            buryCount++;
    }

    return buryCount;
}

int TANK::shootClearance(int targetAngle, int minimumClearance)
{
    int clearance = 2;
    int iXpos, iYpos;
    do
    {
        iXpos = (int) (x + (_global->slope[targetAngle][0] * (GUNLENGTH + clearance)));
        iYpos = (int) (y + (_global->slope[targetAngle][1] * (GUNLENGTH + clearance)));
        if  ((iYpos <= MENUHEIGHT) || (iXpos <= 1) || (iXpos >= (_global->screenWidth - 2)) )
            clearance = minimumClearance;    // done it! There can't be dirt any more!
        else
            clearance++;
    }
    while ((clearance < minimumClearance) && (getpixel(_env->terrain, iXpos, iYpos) == PINK));

    // If we are going for a particular minimumClearance (< screenWidth), it is important whether we hit a wall
    if (minimumClearance < _global->screenWidth)
    {
        int iWall = _env->current_wallType;
        if ( _global->bIsBoxed && (iYpos <= MENUHEIGHT) && ((iWall == WALL_STEEL) || (iWall == WALL_WRAP)) )
            clearance = -1; // Wall hit on ceiling
        if ( ((iXpos <= 1) || (iXpos >= (_global->screenWidth - 2))) && (iWall == WALL_STEEL) )
            clearance = -1; // Wall hit on sides
    }

    return (clearance >= minimumClearance ? 1 : 0);
}

int TANK::isSubClass(int classNum)
{
    if (classNum == TANK_CLASS)
        return TRUE;
    else
        return FALSE;
    //return (PHYSICAL_OBJECT::isSubClass(classNum));
}

/*
This function checks to see if there is a tank directly below this
one. This is to determine if we landed on someone.
The function returns TRUE if we landed on another tank and
FALSE if we did not.
-- Jesse
*/
int TANK::tank_on_tank(GLOBALDATA *global)
{
    int found_tank = FALSE;
    int player_count = 0;
    int delta_x, delta_y;

    while ((player_count < global->numPlayers) && !found_tank)
    {
        // check to make sure this player is alive
        if (global->players[player_count]->tank)
        {
            // make sure this isn't our own tank
            if ((global->players[player_count]->tank->x != x) || (global->players[player_count]->tank->y != y))
            {
                // check to see if tanks are within TANK_WIDTH of each other's x
                delta_x = (int) (x - global->players[player_count]->tank->x);
                delta_y = (int) (y - global->players[player_count]->tank->y);

                if ((abs(delta_x) <= TANKWIDTH) && ( (delta_y < 0) && (delta_y >= -TANKHEIGHT)))
                    found_tank = TRUE;

            }    // end of this is our own tank
        }
        player_count++;
    }

    return found_tank;
}

/*
This function figures out how many points a tank
will repair itself each turn. This is based
on the number of repair kits a player has.
The amount is returned as an int.
-- Jesse
*/
int TANK::Get_Repair_Rate()
{
    int num_kits;
    int repair_units = 0;
    int increase_amount = 5;

    num_kits = player->ni[ITEM_REPAIRKIT];
    while (num_kits > 0)
    {
        repair_units += increase_amount;
        if (increase_amount > 1)
            increase_amount--;
        num_kits--;
    }

    return repair_units;
}

/*
This function tries to move the tank either
left or right one unit. The direction is passed
in.
The function returns TRUE if the tank is moved and
FALSE if something is in the way or the
tank cannot be moved for some reason.
-- Jesse
*/
int TANK::Move_Tank(int direction)
{
    int destination_x;

    // do we have fuel?
    if (player->ni[ITEM_FUEL] < 1)
        return FALSE;

    // see where we want to go
    if (direction == DIR_RIGHT)
        destination_x = (int) (x + TANKWIDTH - 2);
    else
        destination_x = (int) (x - (TANKWIDTH + 1) );

    if (_env->landType != LANDTYPE_NONE)
    {
        // check for something in the way
        int pixel = getpixel(_env->terrain, destination_x, (int) (y + (TANKHEIGHT / 2)));
        if (pixel != PINK)
            return FALSE;
    }

    // move tank
    if (direction == DIR_RIGHT)
        x++;
    else
        x--;

    player->ni[ITEM_FUEL]--;

    return TRUE;
}

void TANK::setEnvironment(ENVIRONMENT *env)
{
    if (!_env || (_env != env))
    {
        _env = env;
        _index = _env->addObject(this);
    }
}

/*
Give credit to the tank who killed us.
*/
void TANK::Give_Credit(GLOBALDATA *global)
{
    if (creditTo)
    {
        // we shot ourselves
        if (creditTo == player)
            creditTo->money -= (int) global->scoreUnitSelfDestroy;
        else   // we were killed by someone else
            creditTo->money += (int) global->scoreUnitDestroyBonus;

        // avoid over-flow
        if (creditTo->money < 0)
            creditTo->money = 0;
        creditTo = NULL;
    }
}
