#include <stdio.h>
#include "main.h"
#include "team.h"
#include "files.h"
#include "satellite.h"
#include "update.h"
#include "network.h"
#include "land.h"

#include "floattext.h"
#include "explosion.h"
#include "beam.h"
#include "missile.h"
#include "decor.h"
#include "teleport.h"
#include "sky.h"

#include "gameloop.h"

#ifdef NEW_GAMELOOP

int game(GLOBALDATA *global, ENVIRONMENT *env)
{
    int humanPlayers = 0;
    int skippingComputerPlay = FALSE;
    int team_won = NO_WIN;
    int my_class;
    int screen_update = TRUE;
    int text_bounce = 0, text_delta = 1;
    VIRTUAL_OBJECT *my_object;
    TANK *my_tank, *a_tank;
    MISSILE *missile;
    TELEPORT *teleport;
    DECOR *decor;
    BEAM *beam;
    EXPLOSION *explosion;
    FLOATTEXT *floattext;
    SATELLITE *satellite = NULL;
    int count;
    int winner = -1, done = FALSE;
    int stuff_happening = FALSE;
    int AI_clock = 0;
    int fire = FALSE;
    int roundEndCount = 0;
    int game_speed = (int) global->frames_per_second * GAME_SPEED / 60;
    int next_tank = FALSE;
    
    // adjust for Windows
#ifdef WIN32
    game_speed /= 1000;
#endif

    global->computerPlayersOnly = FALSE;
    // clear surface
    for (count = 0; count < global->screenWidth; count++)
        env->done[count] = 0;

    env->newRound();
    env->Set_Wall_Colour();
    global->bIsBoxed = env->Get_Boxed_Mode(global);
    global->dMaxVelocity = global->Calc_Max_Velocity();

    for (count = 0; count <global->numPlayers; count++)
        global->players[count]->newRound();

    // clear floating text
    for (count = 0; count < MAX_OBJECTS; count++)
    {
        if (env->objects[count] && (env->objects[count]->isSubClass(FLOATTEXT_CLASS)))
            ((FLOATTEXT *)env->objects[count])->newRound();
    }

    // everyone gets to buy stuff
    buystuff(global, env);
    set_level_settings(global, env);

    for (count = 0; count < global->numPlayers; count++)
    {
        global->players[count]->exitShop();
        if ((global->players[count]->type == HUMAN_PLAYER) || (global->players[count]->type == NETWORK_CLIENT))
            humanPlayers++;
        if (global->players[count]->tank)
        {
            // global->players[count]->tank->newRound();
            global->players[count]->tank->flashdamage = 0;
            global->players[count]->tank->boost_up_shield();
        }
        
    }

    // set_level_settings(global, env);
    env->stage = STAGE_AIM;
    fi = global->updateMenu = TRUE;
    global->window.x = 0;
    global->window.y = 0;
    global->window.w = global->screenWidth - 1;
    global->window.h = global->screenHeight - 1;

    // set wind
    if ( (int) env->windstrength != 0)
        env->wind = (float) (rand() % (int)env->windstrength) - (env->windstrength / 2);
    else
        env->wind = 0;
    env->lastwind = env->wind;

    // create satellite
    if (env->satellite)
        satellite = new SATELLITE(global, env);
    if (satellite)
        satellite->Init();

    // get some mood music
    global->background_music = global->Load_Background_Music();
    if (global->background_music)
        play_sample((SAMPLE *) global->background_music, 255, 128, 1000, TRUE);

    my_tank = env->order[0];

    // init stuff complete. Get down to playing
    while ((!done) && (global->get_command() != GLOBAL_COMMAND_QUIT))
    {
        // see how long we have been skipping
        if (skippingComputerPlay)
        {
            // advance clock
            if (global->Check_Time_Changed())
                AI_clock++;

            if ((AI_clock > MAX_AI_TIME) && (winner == -1))
            {
                // in over-time, kill all tanks
                count = 0;
                while (count < global->numPlayers)
                {
                    if ((global->players[count]) && (global->players[count]->tank))
                        global->players[count]->tank->l = 0;
                    count++;
                }
            }
        }

        global->currTank = my_tank;
        // move through all objects, updating them
        int explosion_in_progress = FALSE;
        count = 0;
        while (count < MAX_OBJECTS)
        {
            my_object = env->objects[count];
            if (my_object)
            {
                my_class = my_object->getClass();
                if (my_class == DECOR_CLASS)
                {
                    decor = (DECOR *) my_object;
                    decor->applyPhysics();
                    if (decor->destroy)
                    {
                        decor->requireUpdate();
                        decor->update();
                        delete decor;
                    }
                }
                else if (my_class == EXPLOSION_CLASS)
                {
                    explosion = (EXPLOSION *) my_object;
                    if (explosion->bIsWeaponExplosion)
                        stuff_happening = TRUE;
                    explosion->explode();
                    explosion_in_progress = TRUE;
                    explosion->applyPhysics();
                    if (explosion->destroy)
                    {
                        explosion->requireUpdate();
                        explosion->update();
                        delete explosion;
                    }
                }

                else if (my_class == TELEPORT_CLASS)
                {
                    teleport = (TELEPORT *) my_object;
                    stuff_happening = TRUE;
                    teleport->applyPhysics();
                    if (teleport->destroy)
                    {
                        teleport->requireUpdate();
                        teleport->update();
                        delete teleport;
                    }
                }
                else if (my_class == MISSILE_CLASS)
                {
                    TANK *shooting_tank = NULL;

                    missile = (MISSILE *) my_object;
                    stuff_happening = TRUE;
                    env->am++;
                    missile->hitSomething = 0;
                    missile->applyPhysics();
                    missile->triggerTest();
                    shooting_tank = missile->Check_SDI(global);
                    if (shooting_tank)
                    {
                        // (shooting_tank->x < missile->x) ? angle_to_fire = 135 : angle_to_fire = 225;
                        int angle_to_fire = atan2(std::abs(shooting_tank->y - 10 - missile->y), (missile->x - shooting_tank->x)) * 180 / 3.14159265 + 90;
                        new BEAM(global, env, shooting_tank->x, shooting_tank->y - 10, angle_to_fire, SML_LAZER);
                        missile->trigger();
                    }
                    if (missile->destroy)
                    {
                        missile->requireUpdate();
                        missile->update();
                        delete missile;
                    }
                }
                else if (my_class == BEAM_CLASS)
                {
                    beam = (BEAM *) my_object;
                    // As bots should not target while a laser is shot:
                    stuff_happening = TRUE;
                    beam->applyPhysics();
                    if (beam->destroy)
                    {
                        beam->requireUpdate();
                        beam->update();
                        delete beam;
                    }
                }
                else if (my_class == FLOATTEXT_CLASS)
                {
                    floattext = (FLOATTEXT *) my_object;
                    floattext->applyPhysics();
                    if (floattext->destroy)
                    {
                        floattext->requireUpdate();
                        floattext->update();
                        delete floattext;
                        env->make_fullUpdate(); // ...kill remaining texts!
                    }
                }
            }        // end of if we have an object

            count++;
        }       // end of updating envirnoment objects

        if (satellite)
            satellite->Move();

        // move land
        slideLand(global, env);

        // seems like a good place to update tanks
        for (count = 0; count < global->numPlayers; count++)
        {
            a_tank = global->players[count]->tank;
            if (a_tank)
            {
                // see if we are doing a volly
                if (a_tank->fire_another_shot)
                {
                    if (!(a_tank->fire_another_shot % VOLLY_DELAY))
                        a_tank->activateCurrentSelection();
                    a_tank->fire_another_shot--;
                }

                if (a_tank->flashdamage)
                {
                    if (a_tank->flashdamage > 25 || a_tank->l < 1)
                    {
                        a_tank->damage = 0;
                        a_tank->flashdamage = 0;
                        a_tank->requireUpdate();
                    }
                }

                // update position
                a_tank->pen = 0;
                a_tank->applyPhysics();

                if ((a_tank->l <= 0) && (!explosion_in_progress))    // dead tank
                {
                    a_tank->explode();
                    a_tank->Give_Credit(global);
                    if (a_tank->destroy)
                    {
                        if ( (a_tank->player) && ((a_tank->player->type == HUMAN_PLAYER)
                             || (a_tank->player->type == NETWORK_CLIENT)) )
                        {
                            humanPlayers--;
                            if ((!humanPlayers) && (global->skipComputerPlay))
                                skippingComputerPlay = TRUE;
                        }

                        a_tank->Destroy();
                        if (my_tank == a_tank)
                            my_tank = NULL;
                        delete a_tank;
                        a_tank = NULL;
                    }
                }

                // if the tank is still alive, adjust its chess-style clock
                if ((global->max_fire_time > 0.0) && (a_tank == my_tank)
                     && a_tank && (a_tank->player->type == HUMAN_PLAYER) && (!env->stage))
                {
                    if (global->Check_Time_Changed())
                    {
                        int ran_out_of_time;
                        ran_out_of_time = a_tank->player->Reduce_Time_Clock();
                        if (ran_out_of_time)
                        {
                            a_tank->player->skip_me = true;
                            a_tank = NULL;
                            fire = FALSE;
                            my_tank = env->Get_Next_Tank(&fire);
                        }
                        global->updateMenu = TRUE;
                    }
                }
                
            }
        }

#ifdef NETWORK
        // check for input from network
        for (count = 0; count < global->numPlayers; count++)
        {
            if (global->players[count]->type == NETWORK_CLIENT)
            {
                global->players[count]->Get_Network_Command();
                global->players[count]->Execute_Network_Command(FALSE);
            }
        }
#endif

        // drop some naturals
        if (!stuff_happening)
        {
            // Added - Switch to the next tank only after there's no stuff happening
            if (next_tank)
            {
                // Flip off our next_tank flag
                next_tank = FALSE;
                
                // Get the next tank and assign it to our current tank
                my_tank = env->Get_Next_Tank(&fire);
                global->currTank = my_tank;
                
                // Make sure the menu and stuff get updated with the new tank
                global->updateMenu = TRUE;
                screen_update = TRUE;
            }
            
            env->stage = STAGE_AIM;
            doNaturals(global, env);
            if (satellite)
                satellite->Shoot();
        }

        // draw top bar
        if (global->updateMenu)
        {
            set_clip_rect(env->db, 0, 0, global->screenWidth - 1, MENUHEIGHT - 1);
            drawTopBar(global, env, env->db);
            global->updateMenu = FALSE;
        }

        set_clip_rect(env->db, 0, MENUHEIGHT, (global->screenWidth-1), (global->screenHeight-1));

        if (screen_update)
        {
            blit(env->sky, env->db, global->window.x, global->window.y - MENUHEIGHT, global->window.x, global->window.y, (global->window.w - global->window.x) + 1, (global->window.h - global->window.y) + 1);
            masked_blit(env->terrain, env->db, global->window.x, global->window.y, global->window.x, global->window.y, (global->window.w - global->window.x) + 1, (global->window.h - global->window.y) + 2);
            // drawTopBar(global, env, env->db);
            int iLeft = 0;
            int iRight = global->screenWidth - 1;
            int iTop = MENUHEIGHT;
            int iBottom = global->screenHeight - 1;
            set_clip_rect(env->db, 0, 0, iRight, iBottom);
            vline(env->db, iLeft, iTop, iBottom, env->wallColour);    // Left edge
            vline(env->db, iLeft + 1, iTop, iBottom, env->wallColour);    // Left edge
            vline(env->db, iRight, iTop, iBottom, env->wallColour);   // right edge
            vline(env->db, iRight - 1, iTop, iBottom, env->wallColour);   // right edge
            hline(env->db, iLeft, iBottom, iRight, env->wallColour);// bottom edge
            if (global->bIsBoxed)
                hline(env->db, iLeft, iTop, iRight, env->wallColour);// top edge

            env->make_update(0, 0, global->screenWidth, global->screenHeight);
        }
        else
        {
            env->replaceCanvas();
            screen_update = TRUE;
        }
        
        for (count = 0; count < global->numPlayers; count++)
        {
            if ((global->players[count]) && (global->players[count]->tank))
            {
                global->players[count]->tank->framelyAccounting();
                if (my_tank == global->players[count]->tank)
                {
                    text_bounce += text_delta;
                    global->players[count]->tank->drawTank(env->db, text_bounce / 4);
                    if ((text_bounce > MAX_TEXT_BOUNCE) || (text_bounce < 1))
                        text_delta = -text_delta;
                }

                else
                    global->players[count]->tank->drawTank(env->db, 0);
                global->players[count]->tank->update();
            }
        }

        if (global->show_scoreboard)
            drawScoreboard(global, env, env->db, my_tank); // Draw the scoreboard

        // draw all this cool stuff
        count = 0;
        while (count < MAX_OBJECTS)
        {
            my_object = env->objects[count];
            if (my_object)
            {
                my_class = my_object->getClass();
                if (my_class == MISSILE_CLASS)
                {
                    missile = (MISSILE *) my_object;
                    missile->draw(env->db);
                    missile->update();
                }
                else if (my_class == BEAM_CLASS)
                {
                    beam = (BEAM *) my_object;
                    beam->draw(env->db);
                    beam->update();
                }
                else if (my_class == EXPLOSION_CLASS)
                {
                    explosion = (EXPLOSION *) my_object;
                    explosion->draw(env->db);
                    explosion->update();
                }
                else if (my_class == TELEPORT_CLASS)
                {
                    teleport = (TELEPORT *) my_object;
                    if (teleport->object)
                        teleport->draw(env->db);
                    teleport->update();
                }
                else if (my_class == DECOR_CLASS && !skippingComputerPlay)
                {
                    decor = (DECOR *) my_object;
                    decor->draw(env->db);
                    decor->update();
                }
                else if (my_class == FLOATTEXT_CLASS)
                {
                    floattext = (FLOATTEXT *) my_object;
                    floattext->draw(env->db);
                    floattext->requireUpdate();
                    floattext->update();
                }
            }        // end of if we have an object

            count++;
        }       // end of while drawing objects

        if (satellite)
            satellite->Draw(env->db);
        env->do_updates();

        // it is possible our tank is dead, make sure we have one
        if (!my_tank)
        {
            my_tank = env->Get_Next_Tank(&fire);
            fire = FALSE;
        }

        // get user input
        if (my_tank && (env->stage < STAGE_ENDGAME))
        {
            PLAYER *my_player;
            int control_result;

            global->updateMenu = FALSE;
            my_player = my_tank->player;
            // my_tank->reactivate_shield();
            control_result = my_player->controlTank();
            if (control_result == CONTROL_QUIT)
                done = TRUE;
            else if (control_result == CONTROL_FIRE)
            {
                // Set the flag to indicate we're ready for our next tank's turn
                // That will happen after stuff_happening is done
                next_tank = TRUE;
                
                if (global->turntype != TURN_SIMUL)
                    fire = TRUE;
            }
            else if ((control_result == CONTROL_SKIP) && (!humanPlayers))
                skippingComputerPlay = TRUE;

            // if ((fire) && (env->stage == STAGE_AIM))
            if (fire)
            {
                env->stage = STAGE_FIRE;
                if (my_tank)
                    my_tank->reactivate_shield();
                doLaunch(global, env);
                fire = FALSE;
                // screen_update = TRUE;
            }
            // else
            // screen_update = FALSE;

            screen_update = FALSE;
            if (control_result)
                global->updateMenu = TRUE;
        }     // end of controling my tank

        if ((!skippingComputerPlay) && (env->stage != STAGE_ENDGAME))
        {
#ifdef LINUX
            usleep(game_speed);
#endif
#ifdef MACOSX
            usleep(game_speed);
#endif
#ifdef WIN32
            Sleep(game_speed);
#endif
        }
        stuff_happening = FALSE;
        // check for winner
        if (env->stage < STAGE_ENDGAME)
        {
            team_won = Team_Won(global);
            if (team_won == NO_WIN)
                winner = global->Check_For_Winner();

            if ((winner >= 0) || (winner == -2) || (team_won != NO_WIN))
                env->stage = STAGE_ENDGAME;
        }

        if (env->stage == STAGE_ENDGAME)
        {
            roundEndCount++;
            if (roundEndCount >= WAIT_AT_END_OF_ROUND)
                done = TRUE;
            screen_update = FALSE;
        }
        if (global->close_button_pressed)
            done = TRUE;
    }    // end of game loop

    // credit winners
    if (team_won != NO_WIN)
        winner = team_won;
    global->Credit_Winners(winner);

    // display winning results
    while (keypressed())
        readkey();     // clear the buffer

    // check to see if we have a winner or we just bailed out early
    if ((winner != -1) && (!global->demo_mode) && (!global->close_button_pressed))
    {
        showRoundEndScoresAt(global, env, env->db,
                             global->screenWidth / 2, global->screenHeight / 2, winner);
        env->do_updates();

        while ((!keypressed()) && (!global->demo_mode))
            LINUX_SLEEP;
        readkey();
    }
    // else
    // {
    global->window.x = global->window.y = 0;
    global->window.w = global->screenWidth - 1;
    global->window.h = global->screenHeight - 1;
    set_clip_rect(env->db, 0, 0, global->window.w, global->window.h);
    // }

    // clean up
    if (satellite)
        delete satellite;

    // remove existing tanks etc
    for (count = 0; count < global->numPlayers; count++)
    {
        if (global->players[count]->tank)
        {
            global->players[count]->Reclaim_Shield();
            delete global->players[count]->tank;
        }
    }

    return done;
}

#endif
