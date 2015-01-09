#ifdef NETWORK

#include "menu.h"
#include "button.h"
#include "team.h"
#include "files.h"
#include "satellite.h"
#include "update.h"
#include "network.h"
#include "client.h"
#include "beam.h"
#include "explosion.h"
#include "missile.h"
#include "teleport.h"
#include "floattext.h"

#include "sky.h"
#ifdef THREADS
#include <pthread.h>
#endif





// Here we try to match the buffer with an action. We then attempt to
// perform the action. Remember, this is a command from the server, so
// it is either giving us some info or telling us to create something.
int Parse_Client_Data(GLOBALDATA *global, ENVIRONMENT *env, char *buffer)
{
   char args[CLIENT_ARGS][BUFFER_SIZE];
   int dest_string;
   int line_length = strlen(buffer);
   int source_index = 0, dest_index = 0;
  
   // clear buffers
   for (dest_string = 0; dest_string < CLIENT_ARGS; dest_string++)
       memset(args[dest_string], '\0', BUFFER_SIZE);

   dest_string = 0; 
   // copy buffer into cmd and argument variables
   while ( ( source_index < line_length ) && (dest_string < CLIENT_ARGS) )
   { 
       char letter = buffer[source_index];
       if ( letter == ' ' )
       {
           letter = '\0';
           args[dest_string][dest_index] = letter;
           dest_index = 0;
           dest_string++;
       }
       else
       {
          args[dest_string][dest_index] = letter;
          dest_index++;
       }
       source_index++;
   }
  
   // let us see what we have
   if (! strcmp(args[0], "SERVERVERSION") )
   {
       if (! strcmp(args[1], VERSION) )
          printf("Server version matchs us. OK.\n");
       else
          printf("Server version is %s, we are %s. This is likely to cause problems.\n",
                 args[1], VERSION);
       return TRUE;
   }
   else if (! strcmp(args[0], "CURRENTPOSITION") )
   {
      if ( (global->client_player) && (global->client_player->tank) )
      {
          sscanf(args[1], "%lf", &(global->client_player->tank->x));
          sscanf(args[2], "%lf", &(global->client_player->tank->y));
      }
   }
   else if (! strcmp(args[0], "BEAM") )
   {
      double my_x, my_y;
      int my_angle, my_type;
      sscanf(args[1], "%lf", &my_x);
      sscanf(args[2], "%lf", &my_y);
      sscanf(args[3], "%d", &my_angle);
      sscanf(args[4], "%d", &my_type);
      new BEAM(global, env, my_x, my_y, my_angle, my_type);
   }
   else if (! strcmp(args[0], "BOXED"))
   {
       int got_box;
       sscanf(args[1], "%d", &got_box);
       if (got_box)
           global->bIsBoxed = true;
       else
           global->bIsBoxed = false;
       return TRUE;
   }
   else if (! strcmp(args[0], "EXPLOSION") )
   {
        double my_x, my_y;
        int my_type;
        sscanf(args[1], "%lf", &my_x);
        sscanf(args[2], "%lf", &my_y);
        sscanf(args[3], "%d", &my_type);
        new EXPLOSION(global, env, my_x, my_y, my_type);
        return FALSE;
   }
   else if (! strcmp(args[0], "ITEM"))
   {
       int item_index, amount;
       sscanf(args[1], "%d", &item_index);
       sscanf(args[2], "%d", &amount);
       if ( (item_index >= 0) && (item_index < ITEMS) &&
            (amount >= 0) && (amount <= 99) )
       {
          global->client_player->ni[item_index] = amount;
       }
       if (item_index == (ITEMS - 1) )
          return TRUE;
   }
   else if (! strcmp(args[0], "HEALTH") )
   {
       int tank_index;
       int health, shield, shield_type;

       sscanf(args[1], "%d", &tank_index);
       if (tank_index >= 0)
       {
           char some_text[32];
          sscanf(args[2], "%d", &health );
          sscanf(args[3], "%d", &shield );
          sscanf(args[4], "%d", &shield_type);
          global->players[tank_index]->tank->l = health;
          global->players[tank_index]->tank->sh = shield;
          global->players[tank_index]->tank->sht = shield_type;
          // set the text over the tank
          sprintf(some_text, "%d", health);
          global->players[tank_index]->tank->healthText->set_text(some_text);
          global->players[tank_index]->tank->healthText->set_color(global->players[tank_index]->color);
          sprintf(some_text, "%d", shield);
          global->players[tank_index]->tank->shieldText->set_text(some_text);
          global->players[tank_index]->tank->healthText->set_color(global->players[tank_index]->color);
       }
       if (tank_index == (global->numPlayers - 1) )
          return TRUE;
       else
          return FALSE;
   }
   else if (! strcmp(args[0], "WIND") )
   {
       sscanf(args[1], "%lf", & (env->wind) );
       return TRUE;
   }
   else if (! strcmp(args[0], "MISSILE") )
   {
       int my_type;
       double my_x, my_y, delta_x, delta_y;
       MISSILE *missile;
       sscanf(args[1], "%lf", &my_x);
       sscanf(args[2], "%lf", &my_y);
       sscanf(args[3], "%lf", &delta_x);
       sscanf(args[4], "%lf", &delta_y);
       sscanf(args[5], "%d", &my_type);
       missile = new MISSILE(global, env, my_x, my_y, delta_x, delta_y, my_type);
       if (! missile)
         printf("Attempted to create missile failed in client code.\n");
       return FALSE;
   }
   else if (! strcmp(args[0], "NUMPLAYERS") )
   {
      int counter;
      sscanf(args[1], "%d", & (global->numPlayers) );
      // create the players in question
      for (counter = 0; counter < global->numPlayers; counter++)
      {
          global->players[counter] = new PLAYER(global, env);
          global->players[counter]->tank = new TANK(global, env);
          global->players[counter]->tank->player = global->players[counter];
          global->players[counter]->tank->initialise();
          global->players[counter]->tank->nameText->set_text("");
      }
      return TRUE;
   }
   // ping is a special case where we do not do anything it is just
   // making sure we are still here because we are not talking
   else if (! strcmp(args[0], "PING"))
   {
       return FALSE;
   }
   else if (! strcmp(args[0], "PLAYERNAME") )
   {
       int number;
       sscanf(args[1], "%d", &number);
       if ( (number < global->numPlayers) && (number >= 0) )
           global->players[number]->setName(args[2]);
       if (number == (global->numPlayers - 1) )
          return TRUE;    
   }
   else if (! strcmp(args[0], "REMOVETANK") )
   {
       int index;
       sscanf(args[1], "%d", &index);
       if ( (index >= 0) && (index < global->numPlayers) )
       {
           // make sure this tank exists before we get rid of it
           if ( global->players[index]->tank )
           {
               delete global->players[index]->tank;
               global->players[index]->tank = NULL;
           }
       }
   }
   else if (! strcmp(args[0], "ROUNDS") )
   {
       sscanf(args[1], "%lf", &global->rounds);
       sscanf(args[2], "%d", &global->currentround);
       return TRUE;
   }
   else if (! strcmp(args[0], "SURFACE") )
   {
       int x, y;
       int index;
       int colour_change = 0;
       int green = 150;
       int my_height;

       sscanf(args[1], "%d", &x);
       sscanf(args[2], "%d", &y);
       env->surface[x] = y;
       my_height = global->screenHeight - y;
       my_height = my_height / 50;      // ratio of change
       // fill in terrain...
       for (index = y; index < global->screenHeight; index++)
       {
           putpixel(env->terrain, x, index, makecol(0, green, 0));
           colour_change++;
           if (colour_change >= my_height)
           {
              colour_change = 0;
              green--;
           }
       }
       if (x >= (global->screenWidth - 1) )
         return TRUE;
   }
   else if (! strcmp(args[0], "SCREEN") )
   {
       int width, height;

       sscanf(args[1], "%d", &width);
       sscanf(args[2], "%d", &height);
       if ( (width == global->screenWidth) &&
            (height == global->screenHeight) )
           printf("Host's screen resolution matches ours.\n");
       else
       {
           printf("Host's screen resolution is %d by %d.\n", width, height);
           printf("Ours is %d by %d. This is going to cause problems!\n",
                  global->screenWidth, global->screenHeight);
        }
       return TRUE;
   }
   else if (! strcmp(args[0], "TANKPOSITION") )
   {
       int player_number, x, y;
       PLAYER *my_player;

       sscanf(args[1], "%d", &player_number);
       my_player = global->players[player_number];
       if ( (my_player) && (my_player->tank) )
       {
           sscanf(args[2], "%d", &x);
           sscanf(args[3], "%d", &y);
           my_player->tank->x = x;
           my_player->tank->y = y;
       }
       if (player_number == (global->numPlayers - 1))
         return TRUE;
   }
   else if (! strcmp(args[0], "TEAM") )
   {
       int player_number;
       double the_team;
       sscanf(args[1], "%d", &player_number);
       sscanf(args[2], "%lf", & the_team);
       if ( (the_team < global->numPlayers) && (the_team >= 0) )
       {
           int colour = 0;
            global->players[player_number]->team = the_team;
            if (the_team == TEAM_JEDI)
                colour = makecol(0, 255, 0);
            else if (the_team == TEAM_SITH)
                colour = makecol(255, 0, 255);
            else if (the_team == TEAM_NEUTRAL)
                colour = makecol(0, 0, 255);
            if (global->players[player_number] == global->client_player)
                colour = makecol(255, 0, 0);
            global->players[player_number]->color = colour;
       }
       if (player_number == (global->numPlayers - 1) )
          return TRUE; 
   }
   else if (! strcmp(args[0], "TELEPORT") )
   {
       int player_num;
       int new_x, new_y;

       sscanf(args[1], "%d", &player_num);
       sscanf(args[2], "%d", &new_x);
       sscanf(args[3], "%d", &new_y);
       if ( (player_num >= 0) && (player_num < global->numPlayers) &&
            (global->players[player_num]->tank) )
       {
          new TELEPORT(global, env, global->players[player_num]->tank,
                       new_x, new_y, TANKHEIGHT * 4 + GUNLENGTH, 120);
       }
           
   }
   else if (! strcmp(args[0], "WALLTYPE"))
   {
        sscanf(args[1], "%d", &(env->current_wallType));
        switch (env->current_wallType)
        {
           case WALL_RUBBER:
           env->wallColour = makecol(0, 255, 0); // GREEN;
           break;
           case WALL_STEEL:
           env->wallColour = makecol(255, 0, 0); // RED;
           break;
           case WALL_SPRING:
           env->wallColour = makecol(0, 0, 255); //BLUE;
           break;
           case WALL_WRAP:
           env->wallColour = makecol(255, 255, 0); // YELLOW;
           break;
        }
       return TRUE;
   }
   else if (! strcmp(args[0], "WEAPON") )
   {
       int weapon_index, amount;
       sscanf(args[1], "%d", &weapon_index);
       sscanf(args[2], "%d", &amount);
       if ( (weapon_index >= 0) && (weapon_index < WEAPONS) &&
            (amount >= 0) && (amount <= 99) )
       {
            global->client_player->nm[weapon_index] = amount;
       }
       if (weapon_index == (WEAPONS - 1))
          return TRUE;
   }
   else if (! strcmp(args[0], "YOUARE") )
   {
       int index;
       sscanf(args[1], "%d", &index );
       if ( (index >= 0) && (index < global->numPlayers) )
       {
          global->client_player = global->players[index];
          global->currTank = global->client_player->tank;
       }
       return TRUE;
   }

   return FALSE;
}




void Create_Sky(ENVIRONMENT *env, GLOBALDATA *global)
{
 if ( (env->custom_background) && (env->bitmap_filenames) )
  {
     if ( env->sky) destroy_bitmap(env->sky);
     env->sky = load_bitmap( env->bitmap_filenames[ rand() % env->number_of_bitmaps ], NULL);
  }

    if ( (! env->custom_background) || (! env->sky) )
    {
      if ( env->sky ) destroy_bitmap(env->sky);
#ifdef THREADS
      if (env->get_waiting_sky())
      {
           env->sky = env->get_waiting_sky();
           env->lock(env->waiting_sky_lock);
           env->waiting_sky = NULL;
           env->unlock(env->waiting_sky_lock);
      }
      else
      {
#endif
      env->sky = create_bitmap( global->screenWidth, global->screenHeight - MENUHEIGHT);
      generate_sky (global, env->sky, sky_gradients[global->cursky],
                   (global->ditherGradients ? GENSKY_DITHERGRAD : 0 ) |
                   (global->detailedSky ? GENSKY_DETAILED : 0 )  );
#ifdef THREADS
      }
#endif
  }

}         // end of create sky function



// Send a shot command to the server
int Client_Fire(PLAYER *my_player, int my_socket)
{
   char buffer[256];

   if (!my_player) return FALSE;
   if (! my_player->tank) return FALSE;

   sprintf(buffer, "FIRE %d %d %d", my_player->tank->cw, my_player->tank->a,
                   my_player->tank->p);
   write(my_socket, buffer, strlen(buffer));
   return TRUE;
}


// Adjust our power on the client side
int Client_Power(PLAYER *my_player, int more_or_less)
{
    if ( (my_player) && (my_player->tank) )
    {
        if ( (more_or_less == CLIENT_UP) && (my_player->tank->p < 1996) )
           my_player->tank->p += 5;
        else if ( (more_or_less == CLIENT_DOWN) && (my_player->tank->p > 5) )
           my_player->tank->p -= 5;
        return TRUE;
    }
    return FALSE;
}



int Client_Angle(PLAYER *my_player, int left_or_right)
{
    if (! my_player) return FALSE;
    if (! my_player->tank) return FALSE;

    if ( (left_or_right == CLIENT_LEFT) && (my_player->tank->a < 270) )
         my_player->tank->a++;
    else if ( (left_or_right == CLIENT_RIGHT) && (my_player->tank->a > 90) )
         my_player->tank->a--;
    return TRUE;
}


int Client_Cycle_Weapon(PLAYER *my_player, int forward_or_back)
{
   bool found = false;

   if (! my_player->tank)
       return FALSE;

   while (! found)
   {
        if (forward_or_back == CYCLE_FORWARD)
            my_player->tank->cw++;
        else 
            my_player->tank->cw--;

        if (my_player->tank->cw >= THINGS)
           my_player->tank->cw = 0;
        else if (my_player->tank->cw < 0)
           my_player->tank->cw = THINGS - 1;

        // check if we have found a weapon
        if (my_player->tank->cw < WEAPONS)
        {
             if (my_player->nm[my_player->tank->cw])
                 found = true;
        }
        else      // an item
        {
             if ( (item[my_player->tank->cw - WEAPONS].selectable) &&
                  (my_player->ni[my_player->tank->cw - WEAPONS]) )
                 found = true;
        }
   }
   return TRUE;
}



// This function takes an error number and returns a string
// which contains useful information about that error.
// On success, a pointer to char is returned.
// On failure, a NULL is returned.
// The returned pointer does NOT need to be freed.
char *Explain_Error(GLOBALDATA *global, int error_code)
{
    char *my_message = NULL;

    switch (error_code)
    {
       case CLIENT_ERROR_VERSION:
           my_message = global->ingame->complete_text[77];
       break;
       case CLIENT_ERROR_SCREENSIZE:
           my_message = global->ingame->complete_text[78];
       break;
       case CLIENT_ERROR_DISCONNECT:
           my_message = global->ingame->complete_text[79];
       break;
    }

    return my_message;
}




#endif

