#include "button.h"
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
#include "player.h"
#include "tank.h"

#include "sky.h"

// Note: Don't guard everything. Empty compilation units are invalid.
#ifdef NETWORK

// From gameloop.cpp:
void draw_top_bar();

// Here we try to match the buffer with an action. We then attempt to
// perform the action. Remember, this is a command from the server, so
// it is either giving us some info or telling us to create something.
int Parse_Client_Data(char *buffer)
{
   char args[CLIENT_ARGS][BUFFER_SIZE];
   char letter;
   int dest_string;
   int line_length = strlen(buffer);
   int sourceindex = 0, destindex = 0;

   // clear buffers
   for (dest_string = 0; dest_string < CLIENT_ARGS; dest_string++)
       memset(args[dest_string], '\0', BUFFER_SIZE);

   dest_string = 0;
   // copy buffer into cmd and argument variables
   while ( ( sourceindex < line_length ) && (dest_string < CLIENT_ARGS) )
   {
       letter = buffer[sourceindex];
       if ( letter == ' ' )
       {
           letter = '\0';
           args[dest_string][destindex] = letter;
           destindex = 0;
           dest_string++;
       }
       else
       {
          args[dest_string][destindex] = letter;
          destindex++;
       }
       sourceindex++;
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
      if ( (global.client_player) && (global.client_player->tank) )
      {
          sscanf(args[1], "%lf", &(global.client_player->tank->x));
          sscanf(args[2], "%lf", &(global.client_player->tank->y));
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
      new BEAM(nullptr, my_x, my_y, my_angle, my_type, BT_WEAPON);
   }
   else if (! strcmp(args[0], "BOXED"))
   {
       int got_box;
       sscanf(args[1], "%d", &got_box);
       if (got_box)
           env.isBoxed = true;
       else
           env.isBoxed = false;
       return TRUE;
   }
   else if (! strcmp(args[0], "EXPLOSION") )
   {
        double my_x, my_y;
        int my_type;
        sscanf(args[1], "%lf", &my_x);
        sscanf(args[2], "%lf", &my_y);
        sscanf(args[3], "%d", &my_type);
        new EXPLOSION(nullptr, my_x, my_y, 0., 0., my_type, true);
        return FALSE;
   }
   else if (! strcmp(args[0], "ITEM"))
   {
       int itemindex, amount;
       sscanf(args[1], "%d", &itemindex);
       sscanf(args[2], "%d", &amount);
       if ( (itemindex >= 0) && (itemindex < ITEMS) &&
            (amount >= 0) && (amount <= 99) )
       {
          global.client_player->ni[itemindex] = amount;
       }
       if (itemindex == (ITEMS - 1) )
          return TRUE;
   }
   else if (! strcmp(args[0], "HEALTH") )
   {
       int tankindex;
       int health, shield, shield_type;
       char some_text[32];

       sscanf(args[1], "%d", &tankindex);
       if (tankindex >= 0)
       {
          sscanf(args[2], "%d", &health );
          sscanf(args[3], "%d", &shield );
          sscanf(args[4], "%d", &shield_type);
          env.players[tankindex]->tank->l = health;
          env.players[tankindex]->tank->sh = shield;
          env.players[tankindex]->tank->sht = shield_type;
          // set the text over the tank
          sprintf(some_text, "%d", health);
          env.players[tankindex]->tank->healthText.set_text(some_text);
          env.players[tankindex]->tank->healthText.set_color(env.players[tankindex]->color);
          sprintf(some_text, "%d", shield);
          env.players[tankindex]->tank->shieldText.set_text(some_text);
          env.players[tankindex]->tank->healthText.set_color(env.players[tankindex]->color);
       }
       if (tankindex == (env.numGamePlayers - 1) )
          return TRUE;
       else
          return FALSE;
   }
   else if (! strcmp(args[0], "WIND") )
   {
       sscanf(args[1], "%lf", & (global.wind) );
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
       missile = new MISSILE(nullptr, my_x, my_y, delta_x, delta_y, my_type,
							MT_WEAPON, 1, 0);
       if (! missile)
         printf("Attempted to create missile failed in client code.\n");
       return FALSE;
   }
   else if (! strcmp(args[0], "NUMPLAYERS") )
   {
      int counter;
      sscanf(args[1], "%d", & (env.numGamePlayers) );
      // create the players in question
      for (counter = 0; counter < env.numGamePlayers; counter++)
      {
          env.players[counter] = new PLAYER();
          env.players[counter]->tank = new TANK();
          env.players[counter]->tank->player = env.players[counter];
          env.players[counter]->tank->nameText.set_text(nullptr);
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
       if ( (number < env.numGamePlayers) && (number >= 0) )
           env.players[number]->setName(args[2]);
       if (number == (env.numGamePlayers - 1) )
          return TRUE;
   }
   else if (! strcmp(args[0], "REMOVETANK") )
   {
       int index;
       sscanf(args[1], "%d", &index);
       if ( (index >= 0) && (index < env.numGamePlayers) )
       {
           // make sure this tank exists before we get rid of it
           if ( env.players[index]->tank )
           {
               delete env.players[index]->tank;
               env.players[index]->tank = NULL;
           }
       }
   }
   else if (! strcmp(args[0], "ROUNDS") )
   {
       sscanf(args[1], "%u", &env.rounds);
       sscanf(args[2], "%u", &global.currentround);
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
       global.surface[x].store(y);
       my_height = env.screenHeight - y;
       my_height = my_height / 50;      // ratio of change
       // fill in terrain...
       for (index = y; index < env.screenHeight; index++)
       {
           putpixel(global.terrain, x, index, makecol(0, green, 0));
           colour_change++;
           if (colour_change >= my_height)
           {
              colour_change = 0;
              green--;
           }
       }
       if (x >= (env.screenWidth - 1) )
         return TRUE;
   }
   else if (! strcmp(args[0], "SCREEN") )
   {
       int width, height;

       sscanf(args[1], "%d", &width);
       sscanf(args[2], "%d", &height);
       if ( (width == env.screenWidth) &&
            (height == env.screenHeight) )
           printf("Host's screen resolution matches ours.\n");
       else
       {
           printf("Host's screen resolution is %d by %d.\n", width, height);
           printf("Ours is %d by %d. This is going to cause problems!\n",
                  env.screenWidth, env.screenHeight);
        }
       return TRUE;
   }
   else if (! strcmp(args[0], "TANKPOSITION") )
   {
       int player_number, x, y;
       PLAYER *my_player;

       sscanf(args[1], "%d", &player_number);
       my_player = env.players[player_number];
       if ( (my_player) && (my_player->tank) )
       {
           sscanf(args[2], "%d", &x);
           sscanf(args[3], "%d", &y);
           my_player->tank->x = x;
           my_player->tank->y = y;
       }
       if (player_number == (env.numGamePlayers - 1))
         return TRUE;
   }
   else if (! strcmp(args[0], "TEAM") )
   {
       int32_t player_number = 0;
       int32_t  colour = BLACK;
       int the_team;
       sscanf(args[1], "%d", &player_number);
       sscanf(args[2], "%d", & the_team);
       if ( (the_team < env.numGamePlayers) && (the_team >= 0) )
       {
            env.players[player_number]->team =
				static_cast<eTeamTypes>(the_team);
            if (the_team == TEAM_JEDI)
                colour = makecol(0, 255, 0);
            else if (the_team == TEAM_SITH)
                colour = makecol(255, 0, 255);
            else if (the_team == TEAM_NEUTRAL)
                colour = makecol(0, 0, 255);
            if (env.players[player_number] == global.client_player)
                colour = makecol(255, 0, 0);
            env.players[player_number]->color = colour;
       }
       if (player_number == (env.numGamePlayers - 1) )
          return TRUE;
   }
   else if (! strcmp(args[0], "TELEPORT") )
   {
       int player_num;
       int new_x, new_y;

       sscanf(args[1], "%d", &player_num);
       sscanf(args[2], "%d", &new_x);
       sscanf(args[3], "%d", &new_y);
       if ( (player_num >= 0) && (player_num < env.numGamePlayers) &&
            (env.players[player_num]->tank) )
       {
			TANK* lt = env.players[player_num]->tank;
          new TELEPORT(lt, new_x, new_y, lt->getDiameter(), 120, ITEM_TELEPORT);
       }

   }
   else if (! strcmp(args[0], "WALLTYPE"))
   {
        sscanf(args[1], "%d", &(env.current_wallType));
        switch (env.current_wallType)
        {
           case WALL_RUBBER:
           env.wallColour = makecol(0, 255, 0); // GREEN;
           break;
           case WALL_STEEL:
           env.wallColour = makecol(255, 0, 0); // RED;
           break;
           case WALL_SPRING:
           env.wallColour = makecol(0, 0, 255); //BLUE;
           break;
           case WALL_WRAP:
           env.wallColour = makecol(255, 255, 0); // YELLOW;
           break;
        }
       return TRUE;
   }
   else if (! strcmp(args[0], "WEAPON") )
   {
       int weaponindex, amount;
       sscanf(args[1], "%d", &weaponindex);
       sscanf(args[2], "%d", &amount);
       if ( (weaponindex >= 0) && (weaponindex < WEAPONS) &&
            (amount >= 0) && (amount <= 99) )
       {
            global.client_player->nm[weaponindex] = amount;
       }
       if (weaponindex == (WEAPONS - 1))
          return TRUE;
   }
   else if (! strcmp(args[0], "YOUARE") )
   {
       int index;
       sscanf(args[1], "%d", &index );
       if ( (index >= 0) && (index < env.numGamePlayers) )
       {
          global.client_player = env.players[index];
          global.set_curr_tank(global.client_player->tank);
       }
       return TRUE;
   }

   return FALSE;
}



void Create_Sky()
{
	if (env.custom_background && env.bitmap_filenames) {
		if (env.sky)
			destroy_bitmap(env.sky);
		env.sky = load_bitmap(
		       env.bitmap_filenames[ rand() % env.number_of_bitmaps ], nullptr);
	}

	if (!env.custom_background || !env.sky) {
		if (env.sky
		  && ( (env.sky->w  != env.screenWidth)
		    || (env.sky->h != (env.screenHeight - MENUHEIGHT) ) ) ) {
			destroy_bitmap(env.sky);
			env.sky = nullptr;
		}

		if (!env.sky)
			env.sky = create_bitmap(env.screenWidth,
		                               env.screenHeight - MENUHEIGHT);
		generate_sky (nullptr, sky_gradients[global.cursky],
		                     (env.ditherGradients ? GENSKY_DITHERGRAD : 0 )
		                   | (env.detailedSky ? GENSKY_DETAILED : 0 ) );
	}
} // end of create sky function



// Send a shot command to the server
int Client_Fire(PLAYER *my_player, int my_socket)
{
   char buffer[256];

   if (!my_player) return FALSE;
   if (! my_player->tank) return FALSE;

   int32_t towrite, written;

	SAFE_WRITE(my_socket, "FIRE %d %d %d", my_player->tank->cw,
				my_player->tank->a, my_player->tank->p)

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
const char* Explain_Error(int32_t error_code)
{
	switch (error_code) {
		case CLIENT_ERROR_VERSION:
			return env.ingame->Get_Line(77);
			break;
		case CLIENT_ERROR_SCREENSIZE:
			return env.ingame->Get_Line(78);
			break;
		case CLIENT_ERROR_DISCONNECT:
			return env.ingame->Get_Line(79);
			break;
	}

	return nullptr;
}


// Client version of the game
// Really, this loop should do some basic things.
// 1. Find out what the landscape should look like from the server.
// 2. Place tanks on the battle field
// 3. Create missiles, beam weapons and such when the server asks us to
// 4. Get input from the player and forward it to the server.
// 5. Clean up at the end of the round.
//
// Function return TRUE if everything went well or FALSE
// if an error occured.
int Game_Client(int socket_number)
{
    int surface_x = 1, tank_position = 1, team_number = 1, name_number = 1;
    int weapon_number = 1, item_number = 1, tank_health = 1;
    int end_of_round = FALSE, keep_playing = FALSE;
    int game_stage = CLIENT_VERSION;
    char buffer[BUFFER_SIZE];
    int incoming;
    int my_key;
    int time_clock = 0;
    bool screen_update = false;
    int count;      // generic counter
    int stuff_going_down = FALSE;    // explosions, missiles etc on the screen
    VIRTUAL_OBJECT *my_object, *next_obj;
	int32_t class_ = 0;
    bool fired = false;
    int32_t towrite, written;


    clear_to_color (global.terrain, PINK);    // get terrain ready
    clear_to_color(global.canvas, BLACK);

    // clean up old text
	global.getHeadOfClass(CLASS_FLOATTEXT, &my_object);
	while (my_object) {
		my_object->getNext(&next_obj);
		static_cast<FLOATTEXT*>(my_object)->newRound();
		delete my_object;
	}

    Create_Sky();     // so we have a background
	SAFE_WRITE(socket_number, "%s", "VERSION")

    while (! end_of_round)
    {
        // check for waiting input from the server
        incoming = Check_For_Incoming_Data(socket_number);
        if (incoming)
        {
           int bytes_read;

           memset(buffer, '\0', BUFFER_SIZE);
           bytes_read = read(socket_number, buffer, BUFFER_SIZE);
           if (bytes_read > 0)
           {
                // do something with this input
                if (! strncmp(buffer, "CLOSE", 5) )
                {
                   end_of_round = TRUE;
                   keep_playing = FALSE;
                   printf("Got close message.\n");
                   global.client_message = strdup(env.ingame->Get_Line(81));
                }
                else if (! strncmp(buffer, "NOROOM", 6) )
                {
                   end_of_round = TRUE;
                   keep_playing = FALSE;
                   printf("The server is full or the game has not started. Please try again later.\n");
                   global.client_message = strdup(env.ingame->Get_Line(80));
                }
                else if (! strncmp(buffer, "GAMEEND", 7) )
                {
                    end_of_round = TRUE;
                    keep_playing = FALSE;
                    printf("The game is over.\n");
                    if ( strlen(buffer) > 7)
                        global.client_message = strdup(& (buffer[8])) ;
                    else
                        global.client_message = strdup(env.ingame->Get_Line(82));
                }
                else if (! strncmp(buffer, "ROUNDEND", 8) )
                {
                   end_of_round = TRUE;
                   keep_playing = TRUE;
                   printf("Round is over.\n");
                }

                else         // not a special command, parse it
                {
                  if ( Parse_Client_Data(buffer) )
                  {
                      if (game_stage < CLIENT_PLAYING)
                         game_stage++;

                      // Request more information
                      if (game_stage < CLIENT_PLAYING)
                      {
                         switch (game_stage)
                         {
                            case CLIENT_SCREEN: strcpy(buffer, "SCREEN"); break;
                            case CLIENT_WIND: strcpy(buffer, "WIND"); break;
                            case CLIENT_NUMPLAYERS: strcpy(buffer, "NUMPLAYERS"); break;
                            case CLIENT_TANK_POSITION: strcpy(buffer, "TANKPOSITION 0"); break;
                            case CLIENT_SURFACE: strcpy(buffer, "SURFACE 0"); break;
                            case CLIENT_WHOAMI: strcpy(buffer, "WHOAMI"); break;
                            case CLIENT_WEAPONS: strcpy(buffer, "WEAPON 0"); break;
                            case CLIENT_ITEMS:   strcpy(buffer, "ITEM 0"); break;
                            case CLIENT_ROUNDS: strcpy(buffer, "ROUNDS"); break;
                            case CLIENT_TEAMS: strcpy(buffer, "TEAMS 0");
                                               global.updateMenu = TRUE; break;
                            case CLIENT_WALL_TYPE: strcpy(buffer, "WALLTYPE"); break;
                            case CLIENT_BOXED: strcpy(buffer, "BOXED"); break;
                            case CLIENT_NAME: strcpy(buffer, "PLAYERNAME 0"); break;
                            case CLIENT_TANK_HEALTH: strcpy(buffer, "HEALTH 0"); break;
                            default: buffer[0] = '\0';
                         }
                         towrite = strlen(buffer);
                         written = write(socket_number, buffer, strlen(buffer));
                         if (written < towrite)
							fprintf(stderr,
								"%s:%d: Warning: Only %d/%d bytes sent to server\n",
								__FILE__, __LINE__, written, towrite);
                       }   // end of getting more info
                   }    // our game stage went up
                   else  // we got data, but our game stage did not go up
                   {
                       if (fired)
                       {
                           if ( (global.client_player) && (global.client_player->tank) )
                           {
                               fired = false;
                               if (global.client_player->tank->cw < WEAPONS)
								SAFE_WRITE(socket_number, "WEAPON %d",
												global.client_player->tank->cw)
                               else
								SAFE_WRITE(socket_number, "ITEM %d",
												global.client_player->tank->cw - WEAPONS)
                           }
                       }
                       else if (game_stage == CLIENT_SURFACE)
                       {
								SAFE_WRITE(socket_number, "SURFACE %d", surface_x)
                           surface_x++;
                       }
                       else if (game_stage == CLIENT_ITEMS)
                       {
								SAFE_WRITE(socket_number, "ITEM %d", item_number)
                            item_number++;
                       }
                       else if (game_stage == CLIENT_TANK_POSITION)
                       {
								SAFE_WRITE(socket_number, "TANKPOSITION %d", tank_position)
                            tank_position++;
                            if (tank_position >= env.numGamePlayers)
                              tank_position = 0;
                       }
                       else if (game_stage == CLIENT_TANK_HEALTH)
                       {
								SAFE_WRITE(socket_number, "HEALTH %d", tank_health)
                            tank_health++;
                            if (tank_health >= env.numGamePlayers)
                                tank_health = 0;
                       }
                       else if (game_stage == CLIENT_TEAMS)
                       {
								SAFE_WRITE(socket_number, "TEAMS %d", team_number)
                            team_number++;
                       }
                       else if (game_stage == CLIENT_NAME)
                       {
								SAFE_WRITE(socket_number, "PLAYERNAME %d", name_number)
                            name_number++;
                       }
                       else if (game_stage == CLIENT_WEAPONS)
                       {
								SAFE_WRITE(socket_number, "WEAPON %d", weapon_number)
                            weapon_number++;
                       }
                       else if (game_stage == CLIENT_PLAYING)
                       {
                           time_clock++;
                           if (time_clock > 1)   // check positions every few inputs
                           {
                               time_clock = 0;
                               if (surface_x < env.screenWidth)
                               {
                                   game_stage = CLIENT_SURFACE;
								SAFE_WRITE(socket_number, "SURFACE %d", surface_x)
                                   surface_x++;
                               }
                               else
                               {
                                 game_stage = CLIENT_TANK_POSITION;
                                 tank_position = 1;
								SAFE_WRITE(socket_number, "TANKPOSITION %d", 0)
                               }    // game stage stuff
                           }
                       }        // end of playing commands
                   }

                }      // end of we got something besides the close command

           }
           else    // connection was broken
           {
              close(socket_number);
              printf("Server closed connection.\n");
              end_of_round = TRUE;
           }
        }

		class_ = 0;
		while (class_ < CLASS_COUNT) {
			if (CLASS_TANK == class_) {
				++class_;
				continue;
			}

			global.getHeadOfClass(static_cast<eClasses>(class_), &my_object);
			while(my_object) {
				my_object->getNext(&next_obj);

				if (CLASS_EXPLOSION == class_)
					static_cast<EXPLOSION*>(my_object)->explode();

				my_object->applyPhysics();

				if (my_object->destroy) {
					my_object->requireUpdate();
					my_object->update();
					delete my_object;
					if (CLASS_TELEPORT == class_)
						time_clock = 2;
				}

				if ( (CLASS_BEAM      == class_)
				  || (CLASS_MISSILE   == class_)
				  || (CLASS_EXPLOSION == class_)
				  || (CLASS_TELEPORT  == class_) )
					stuff_going_down = TRUE;

				my_object = next_obj;
			}
			++class_;
		}

		global.slideLand();

        // update everything on the screen
        if (global.updateMenu)
          draw_top_bar ();

        if (screen_update) {
			screen_update = false;
			global.make_fullUpdate();
        }
		global.replace_canvas ();

        screen_update = true;

		class_ = 0;
		while (class_ < CLASS_COUNT) {
			global.getHeadOfClass(static_cast<eClasses>(class_), &my_object);
			while(my_object) {
				my_object->draw();
				if (CLASS_FLOATTEXT == class_)
					my_object->requireUpdate();
				my_object->update();
				my_object->getNext(&my_object);
			}
			++class_;
		}

        global.do_updates();


        // check for input from the user
        if ( keypressed() )
        {
           my_key = readkey();
           my_key = my_key >> 8;
           if (my_key == KEY_SPACE)
           {
              Client_Fire(global.client_player, socket_number);
              fired = true;
           }
           else if (my_key == KEY_ESC)
           {
              end_of_round = TRUE;
              close(socket_number);
           }
           else if (my_key == KEY_UP)
           {
              Client_Power(global.client_player, CLIENT_UP);
           }
           else if (my_key == KEY_DOWN)
           {
              Client_Power(global.client_player, CLIENT_DOWN);
           }
           else if (my_key == KEY_LEFT)
           {
              Client_Angle(global.client_player, CLIENT_LEFT);
           }
           else if (my_key == KEY_RIGHT)
           {
              Client_Angle(global.client_player, CLIENT_RIGHT);
           }
           else if ( (my_key == KEY_Z) || (my_key == KEY_BACKSPACE) )
           {
               Client_Cycle_Weapon(global.client_player, CYCLE_BACK);
           }
           else if ( (my_key == KEY_C) || (my_key == KEY_TAB) )
           {
               Client_Cycle_Weapon(global.client_player, CYCLE_FORWARD);
               global.updateMenu = TRUE;
           }

           screen_update = false;
           global.updateMenu = TRUE;
        }

        // pause for a moment
        // if (game_stage < CLIENT_PLAYING)
        if (stuff_going_down)
        {
            LINUX_SLEEP;
            stuff_going_down = FALSE;
        }
    }

    // we should clean up here
    for (count = 0; count < env.numGamePlayers; count++)
    {
       if (env.players[count]->tank)
       {
           delete env.players[count]->tank;
           env.players[count]->tank = NULL;
       }
    }

    return keep_playing;
}


#endif // NETWORK

