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

#include <time.h>
#include "player.h"
#include "globaldata.h"
#include "files.h"
#include "network.h"
#include "team.h"

GLOBALDATA::GLOBALDATA ():dataDir(NULL),configDir(NULL),updates(NULL),lastUpdates(NULL),allPlayers(NULL),
    players(NULL),currTank(NULL),saved_game_list(NULL)
{
  init_curland_lock();

  #ifdef THREADS
  command_lock = (pthread_rwlock_t*) malloc(sizeof(pthread_rwlock_t));
  if (command_lock == NULL)
  {
    printf("%s:%i: Could not allocate memory for command_lock.\n", __FILE__, __LINE__);
  }
  int result = pthread_rwlock_init(command_lock, NULL);
  switch (result)
  {
    case 0:
       //Successfully initialized
       break;
    case EAGAIN:
       //resource lack
       printf("%s:%i: Not enough resources to initialize read-write lock.\n", __FILE__, __LINE__);
       break;
    case ENOMEM:
       //out of memory
       printf("%s:%i: Not enough memory to initialize read-write lock.\n", __FILE__, __LINE__);
       break;
    case EPERM:
       //not authorized
       printf("%s:%i: Not authorized.\n", __FILE__, __LINE__);
       break;
    default:
       //If the switch ever gets to here, something very wrong happened and pthread_rwlock_init returned
       //a random value
       printf("%s:%i: Unknown error code (%i) returned by pthread_rwlock_init.\n", __FILE__, __LINE__, result);
       break;
  }
  #endif
  tank_status = (char *)calloc(128, sizeof(char));
  if (!tank_status)
    {
      perror ( "globaldata.cpp: Failed allocating memory for tank_status in GLOBALDATA::GLOBALDATA");
      // exit (1);
    }

  initialise ();
  language = LANGUAGE_ENGLISH;
  sound = 1.0;
  name_above_tank = TRUE;
  colourDepth = 0;
  client_player = NULL;
  screenWidth = DEFAULT_SCREEN_WIDTH;
  screenHeight = DEFAULT_SCREEN_HEIGHT;
  width_override = height_override = 0;
  temp_screenWidth = screenWidth;
  temp_screenHeight = screenHeight;
  halfWidth = screenWidth / 2;
  halfHeight = screenHeight / 2;
  menuBeginY = (screenHeight - 400) / 2;
  if (menuBeginY < 0) menuBeginY = 0;
  menuEndY = screenHeight - menuBeginY;
  frames_per_second = FRAMES_PER_SECOND;
  numPermanentPlayers = 10;
  violent_death = FALSE;
/*
#ifndef DOS
  cacheCirclesBG = 1;
#endif

#ifdef DOS
  cacheCirclesBG = 0;
#endif
*/
  ditherGradients = 1;
  detailedLandscape = 0;
  detailedSky = 0;
  colour_theme = COLOUR_THEME_CRISPY;
  startmoney = 15000;
  turntype = TURN_RANDOM;
  skipComputerPlay = SKIP_HUMANS_DEAD;
  // dataDir = DATA_DIR;
  Find_Data_Dir();
  os_mouse = 1.0;
  full_screen = FULL_SCREEN_FALSE;
  interest = 1.25;
  scoreHitUnit = 75;
  scoreSelfHit = 0;
  scoreUnitDestroyBonus = 5000;
  scoreUnitSelfDestroy = 0;
  scoreRoundWinBonus = 10000;
  sellpercent = 0.80;
  game_name[0] = '\0';
  load_game = 0.0;
  campaign_mode = 0.0;
  saved_game_index = 0;
  saved_game_list = NULL;
  max_fire_time = 0.0;
  close_button_pressed = false;
  divide_money = 0.0;
  sound_driver = SOUND_AUTODETECT;
  update_string = NULL;
  check_for_updates = 1.0;
  demo_mode = false;
  env = NULL;
  war_quotes = instructions = ingame = NULL;
  gloat = revenge = retaliation = kamikaze = suicide = NULL;
  client_message= NULL;
  show_scoreboard = false;

  updates = new BOX[MAXUPDATES];
  if (!updates)
    {
      perror ( "globaldata.cc: Failed allocating memory for updates in GLOBALDATA::GLOBALDATA");
      // exit (1);
    }
  lastUpdates = new BOX[MAXUPDATES];
  if (!lastUpdates)
    {
      perror ( "globaldata.cc: Failed allocating memory for lastUpdates in GLOBALDATA::GLOBALDATA");
      // exit (1);
    }
  updateCount = 0;
  lastUpdatesCount = 0;

  // players = new PLAYER*[MAXPLAYERS];
  players = (PLAYER **) calloc( MAXPLAYERS, sizeof(PLAYER *) );
  if (!players)
    {
      perror ( "globaldata.cc: Failed allocating memory for players in GLOBALDATA::GLOBALDATA");
      // exit (1);
    }
  numPlayers = 0;
  rounds = 5;

  if (allPlayers) free(allPlayers);   // avoid potential leak
  allPlayers = (PLAYER**) calloc (1, sizeof (PLAYER*));
  if (!allPlayers)
    {
      fprintf (stderr, "Failed allocating memory for players in globaldata.cc\n");
      // exit (1);
    }

  for (int count = 0; count < 360; count++)
    {
      slope[count][0] = sin (count / (180 / PI));
      slope[count][1] = cos (count / (180 / PI));
    }
  slope[270][1] = 0;
  configDir = NULL;
  bIsGameLoaded = true;
  bIsBoxed = false;
  iHumanLessRounds = -1;
  dMaxVelocity = 0.0;	// Will be set in game()
#ifdef DEBUG_AIM_SHOW
  bASD = false; // will be set to true after the first drawing of the map
#endif
  enable_network = 0.0;
#ifdef NETWORK
  listen_port = DEFAULT_LISTEN_PORT;
#endif
  strcpy(server_name, "127.0.0.1");
  strcpy(server_port, "25645");
  play_music = 1.0;
  background_music = NULL;
  music_dir = NULL;
  unicode = NULL;
  regular_font = font;
  draw_background = TRUE;
}



GLOBALDATA::~GLOBALDATA()
{
  int index;

  if (tank_status)
  {
      tank_status[0] = 0;
      free(tank_status);
  }

  index = 0;
  while (sounds[index])
  {
     destroy_sample(sounds[index]);
  }
  free(sounds);

  if (music_dir)
     closedir(music_dir);
  if (unicode)
     destroy_font(unicode);

  if (war_quotes) delete war_quotes;
  if (instructions) delete instructions;
  if (ingame) delete ingame;
  if (gloat) delete gloat;
  if (revenge) delete revenge;
  if (retaliation) delete retaliation;
  if (kamikaze) delete kamikaze;
  if (suicide) delete suicide;
  if (allPlayers) free(allPlayers);
  if (update_string) free(update_string);
  #ifdef THREADS
  if (command_lock)
  {
    int result = pthread_rwlock_destroy(command_lock);
    switch (result)
    {
      case 0:
         //Successfully destroyed
         break;
      case EBUSY:
         //Some thread forgot to unlock the read-write lock
         printf("%s:%i: Can't destroy because command_lock is still locked.\n", __FILE__, __LINE__);
         break;
      case EINVAL:
         //Invalid lock
         printf("%s:%i: The lock is invalid.\n", __FILE__, __LINE__);
         break;
      default:
         //Something went wrong
         printf("%s:%i: Unknown error code (%i) returned by pthread_rwlock_destroy.\n", __FILE__, __LINE__, result);
         break;
    }
    free(command_lock);
  }
  destroy_curland_lock();
  #endif
}
//Destroys curland_lock and frees it. If the lock is NULL or an error occurs, a diagnostic message will be printed.
void GLOBALDATA::destroy_curland_lock() {
  #ifdef THREADS
  if (curland_lock != NULL)
  {
    int result = pthread_mutex_destroy(curland_lock);
    switch (result)
    {
      case 0:
        //Successfully destroyed
        break;
      case EBUSY:
        //Some thread forgot to unlock result
        printf("%s:%i: Lock is still held.\n", __FILE__, __LINE__);
        break;
      case EINVAL:
        //Invalid lock
        printf("%s:%i: Lock is invalid.\n", __FILE__, __LINE__);
        break;
      default:
        printf("%s:%i: Unknown error code (%i) returned by pthread_mutex_destroy.\n", __FILE__, __LINE__, result);
        break;
    }
    free(curland_lock);
  }
  else
  {
    printf("%s:%i: Cannot destroy a null mutex.\n", __FILE__, __LINE__);
  }
  #endif
}
//init_curland_lock() allocates space for curland_lock and then calls pthread_mutex_init to initialize the lock. If the call fails, then this function will print diagnostic messages.
void GLOBALDATA::init_curland_lock() {
  #ifdef THREADS
  curland_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  if (!curland_lock)
  {
    printf("%s:%i: Could not allocate memory for curland_lock.\n", __FILE__, __LINE__);
  }
  int result = pthread_mutex_init(curland_lock, NULL);
  switch (result)
  {
    case 0:
      //Succesfully initialized
      break;
    case EAGAIN:
      //Not enough resources
      printf("%s:%i: Not enough resources to create mutex.\n", __FILE__, __LINE__);
      break;
    case ENOMEM:
      printf("%s:%i: Not enough memory to create mutex.\n", __FILE__, __LINE__);
      break;
    case EPERM:
      printf("%s:%i: Not authorized.\n", __FILE__, __LINE__);
      break;
    case EBUSY:
      printf("%s:%i: The mutex is already initialized.\n", __FILE__, __LINE__);
      break;
    case EINVAL:
      printf("%s:%i: Invalid attribute.\n", __FILE__, __LINE__);
      break;
    default:
      printf("%s:%i: Unknown error code (%i) returned by pthread_mutex_init.\n", __FILE__, __LINE__, result);
      break;
  }
  #endif
}
//lock_curland() locks curland and prints diagnostic messages if the lock operation fails.
//Use get_curland() to read the value instead of directly using lock_curland()/unlock_curland() pairs.
void GLOBALDATA::lock_curland()
{
  #ifdef THREADS
  int result = pthread_mutex_lock(curland_lock);
  switch (result)
  {
    case 0:
      //Got the lock.
      break;
    case EINVAL:
      //Priority too high
      printf("%s:%i: Either this thread's priority is higher than the mutex's priority, or the mutex is uninitialized.\n", __FILE__, __LINE__);
      break;
    case EAGAIN:
      printf("%s:%i: Too many locks on the mutex.\n", __FILE__, __LINE__);
      break;
    case EDEADLK:
      //We have the lock already
      printf("%s:%i: Already have lock.\n", __FILE__, __LINE__);
      break;
    default:
      //What error is this?
      printf("%s:%i: Unknown error code (%i) returned by pthread_mutex_lock.\n", __FILE__, __LINE__, result);
      break;
  }
  #endif
}
//unlock_curland() will unlock curland and print diagnostic messages if the unlock operation fails.
void GLOBALDATA::unlock_curland()
{
  #ifdef THREADS
  int result = pthread_mutex_unlock(curland_lock);
  switch (result)
  {
    case 0:
      //Released the lock
      break;
    case EPERM:
      //Forgot to get a lock on the mutex
      printf("%s:%i: Mutex isn't locked\n", __FILE__, __LINE__);
      break;
    case EINVAL:
      //Uninitialized mutex
      printf("%s:%i: waiting_sky_lock is uninitialized.\n", __FILE__, __LINE__);
      break;
    default:
      //?
      printf("%s:%i: Unknown error code (%i) returned by pthread_mutex_unlock.\n", __FILE__, __LINE__, result);
      break;
  }
  #endif
}
//get_curland() locks curland, reads the value, unlocks curland, then returns the value. The value returned cannot be assigned to.
//Use this only for reading the value of curland, not for writing.
//For writing, use lock_curland()/unlock_curland().
int GLOBALDATA::get_curland()
{
  lock_curland();
  int c = curland;
  unlock_curland();
  return c;
}

//Locks global->command for writing. Unlock with GLOBALDATA::unlock_command().
void GLOBALDATA::wr_lock_command()
{
  #ifdef THREADS
  int result = pthread_rwlock_wrlock(command_lock);
  switch (result)
  {
    case 0:
      //Got the lock.
      break;
    case EINVAL:
      //No lock has been created yet
      printf("%s:%i: Read-write lock is uninitialized.\n", __FILE__, __LINE__);
      break;
    case EDEADLK:
      //We have the lock already
      printf("%s:%i: Can't lock for writing because the lock is already locked for either reading or writing.\n", __FILE__, __LINE__);
      break;
    default:
      //What error is this?
      printf("%s:%i: Unknown error code (%i) returned by pthread_rwlock_wrlock.\n", __FILE__, __LINE__, result);
      break;
  }
  #endif
}
//Unlocks a read or a write lock. Do not try to use this function to unlock a lock acquired by calling get_command().
void GLOBALDATA::unlock_command()
{
  #ifdef THREADS
  int result = pthread_rwlock_unlock(command_lock);
  switch (result)
  {
    case 0:
      //Released the lock
      break;
    case EPERM:
      //We have released all locks
      printf("%s:%i: Can't unlock because no read or write lock is currently being held.\n", __FILE__, __LINE__);
      break;
    case EINVAL:
      //Uninitialized read-write lock
      printf("%s:%i: global->command_lock is uninitialized.\n", __FILE__, __LINE__);
      break;
    default:
      //Something went wrong
      printf("%s:%i: Unknown error code (%i) returned by pthread_rwlock_unlock.\n", __FILE__, __LINE__, result);
      break;
  }
  #endif
}
//Locks global->command for reading, reads value, then unlocks the variable and returns the value.
int GLOBALDATA::get_command()
{
  #ifdef THREADS
  int result = pthread_rwlock_rdlock(command_lock);
  switch (result)
  {
    case 0:
      //Got the lock
      break;
    case EINVAL:
      //No lock has been created.
      printf("%s:%i: *global->command_lock is uninitialized.\n", __FILE__, __LINE__);
      break;
    case EAGAIN:
      //Can't read since already trying to read
      //Maybe locks are not being unlocked?
      printf("%s:%i: Too many read locks already held.\n", __FILE__, __LINE__);
      break;
    case EDEADLK:
      //Obtaining read locks while possessing a write lock is forbidden with read-write locks.
      printf("%s:%i: Already own write lock; can't get read lock.\n", __FILE__, __LINE__);
      break;
    default:
      //What error is this?
      printf("%s:%i: Unknown error code (%i) returned by pthread_rwlock_rdlock.\n", __FILE__, __LINE__, result);
      break;
  }
  #endif
  int c = command;
  unlock_command();
  return c;
}

/*
This function saves the global data to a text file. If all goes
well, TRUE is returned, on error, FALSE is returned.
-- Jesse
*/
int GLOBALDATA::saveToFile_Text( FILE *file)
{
  if (! file) return FALSE;

  setlocale(LC_NUMERIC, "C");

  screenWidth = (int) temp_screenWidth;
  screenHeight = (int) temp_screenHeight;

  fprintf (file, "*GLOBAL*\n");

  fprintf (file, "NUMPLAYERS=%d\n", numPlayers);
  fprintf (file, "ROUNDS=%f\n", rounds);
  fprintf (file, "DITHER=%f\n", ditherGradients);
  fprintf (file, "DETAILEDSKY=%f\n", detailedSky);
  fprintf (file, "DETAILEDLAND=%f\n", detailedLandscape);
  fprintf (file, "STARTMONEY=%f\n", startmoney);
  fprintf (file, "TURNTYPE=%f\n", turntype);
  fprintf (file, "INTEREST=%f\n", interest);
  fprintf (file, "SCOREROUNDWINBONUS=%f\n", scoreRoundWinBonus);
  fprintf (file, "SCOREHITUNIT=%f\n", scoreHitUnit);
  fprintf (file, "SCOREUNITDESTROYBONUS=%f\n", scoreUnitDestroyBonus);
  fprintf (file, "SCOREUNITSELFDESTROY=%f\n", scoreUnitSelfDestroy);
  fprintf (file, "ACCELERATEDAI=%f\n", skipComputerPlay);
  fprintf (file, "SELLPERCENT=%f\n", sellpercent);
  fprintf (file, "ENABLESOUND=%f\n", sound);
  fprintf (file, "SCREENWIDTH=%d\n", screenWidth);
  fprintf (file, "SCREENHEIGHT=%d\n", screenHeight);
  fprintf (file, "OSMOUSE=%f\n", os_mouse);
  fprintf (file, "NUMPERMANENTPLAYERS=%d\n", numPermanentPlayers);
  fprintf (file, "LANGUAGE=%f\n", language);
  fprintf (file, "COLOURTHEME=%f\n", colour_theme);
  fprintf (file, "FRAMES=%f\n", frames_per_second);
  fprintf (file, "VIOLENTDEATH=%f\n", violent_death);
  fprintf (file, "MAXFIRETIME=%f\n", max_fire_time);
  fprintf (file, "DIVIDEMONEY=%f\n", divide_money);
  fprintf (file, "CHECKUPDATES=%f\n", check_for_updates);
  fprintf (file, "NETWORKING=%f\n", enable_network);
  fprintf (file, "LISTENPORT=%f\n", listen_port);
  fprintf (file, "SOUNDDRIVER=%f\n", sound_driver);
  fprintf (file, "PLAYMUSIC=%f\n", play_music);
  fprintf (file, "FULLSCREEN=%f\n", full_screen);
  fprintf (file, "SCOREBOARD=%d\n", show_scoreboard);
  fprintf (file, "***\n");
  return TRUE;
}

/*
This function loads global settings from a text
file. The function returns TRUE on success and FALSE if
any erors are encountered.
-- Jesse
*/

int GLOBALDATA::loadFromFile_Text (FILE *file)
{
  char line[MAX_CONFIG_LINE];
  int equal_position, line_length;
  char field[MAX_CONFIG_LINE], value[MAX_CONFIG_LINE];
  char *result = NULL;
  bool done = false;
  double sound_bookmark = 1.0;

  setlocale(LC_NUMERIC, "C");
  if (! sound)
    sound_bookmark = sound;

 // read until we hit line "*ENV*" or "***" or EOF
  do
    {
      result = fgets(line, MAX_CONFIG_LINE, file);
      if (! result)     // eof
        return FALSE;
      if (! strncmp(line, (char *)"***", 3) )     // end of record
        return FALSE;
    }
  while ( strncmp(line, (char *)"*GLOBAL*", 5) );     // read until we hit new record

  while ( (result) && (!done) )
    {
      // read a line
      memset(line, '\0', MAX_CONFIG_LINE);
      result = fgets(line, MAX_CONFIG_LINE, file);
      if (result)
        {
          // if we hit end of the record, stop
          if (! strncmp(line, (char *)"***", 3) )
            {
              return TRUE;
            }
          // find equal sign
          line_length = strlen(line);
          // strip newline character
          if ( line[line_length - 1] == '\n')
            {
              line[line_length - 1] = '\0';
              line_length--;
            }
          equal_position = 1;
          while ( ( equal_position < line_length) && (line[equal_position] != '=') )
            equal_position++;
          // make sure we have valid equal sign

          if ( equal_position <= line_length )
            {
              // seperate field from value
              memset(field, '\0', MAX_CONFIG_LINE);
              memset(value, '\0', MAX_CONFIG_LINE);
              strncpy(field, line, equal_position);
              strcpy(value, & (line[equal_position + 1]));
              if (! strcasecmp(field, "numplayers") )
                sscanf(value, "%d", &numPlayers);
              else if (! strcasecmp(field, "rounds") )
                sscanf(value, "%lf", &rounds);
              else if (! strcasecmp(field, "dither"))
                sscanf(value, "%lf", &ditherGradients);
              else if (! strcasecmp(field, "detailedsky"))
                sscanf(value, "%lf", &detailedSky);
              else if (! strcasecmp(field, "detailedland"))
                sscanf(value, "%lf", &detailedLandscape);
              else if (! strcasecmp(field, "startmoney"))
                sscanf(value, "%lf", &startmoney);
              else if (! strcasecmp(field, "turntype"))
                sscanf(value, "%lf", &turntype);
              else if (! strcasecmp(field, "interest"))
                sscanf(value, "%lf", &interest);
              else if (! strcasecmp(field, "scoreroundwinbonus"))
                sscanf(value, "%lf", &scoreRoundWinBonus);
              else if (! strcasecmp(field, "scorehitunit"))
                sscanf(value, "%lf", &scoreHitUnit);
              else if (! strcasecmp(field, "scoreunitdestroybonus"))
                sscanf(value, "%lf", &scoreUnitDestroyBonus);
              else if (! strcasecmp(field, "scoreunitselfdestroy"))
                sscanf(value, "%lf", &scoreUnitSelfDestroy);
              else if (! strcasecmp(field, "acceleratedai"))
                sscanf(value, "%lf", &skipComputerPlay);
              else if (! strcasecmp(field, "sellpercent"))
                sscanf(value, "%lf", &sellpercent);
              else if (! strcasecmp(field, "enablesound"))
                sscanf(value, "%lf", &sound);
              else if (! strcasecmp(field, "screenwidth"))
                sscanf(value, "%d", &screenWidth);
              else if (! strcasecmp(field, "screenheight"))
                sscanf(value, "%d", &screenHeight);
              else if (! strcasecmp(field, "OSMOUSE"))
                sscanf(value, "%lf", &os_mouse);
              else if (! strcasecmp(field, "numpermanentplayers"))
                sscanf(value, "%d", &numPermanentPlayers);
              else if (! strcasecmp(field, "language") )
                sscanf(value, "%lf", &language);
              else if (! strcasecmp(field, "colourtheme") )
                sscanf(value, "%lf", &colour_theme);
              else if (! strcasecmp(field, "frames") )
                sscanf(value, "%lf", &frames_per_second);
              else if (! strcasecmp(field, "violentdeath") )
                sscanf(value, "%lf", &violent_death);
              else if (! strcasecmp(field, "maxfiretime") )
                sscanf(value, "%lf", &max_fire_time);
              else if (! strcasecmp(field, "dividemoney") )
                sscanf(value, "%lf", &divide_money);
              else if (!strcasecmp(field, "checkupdates")) 
                sscanf(value, "%lf", &check_for_updates);
              else if (!strcasecmp(field, "networking"))
                sscanf(value, "%lf", &enable_network);
              else if (!strcasecmp(field, "listenport"))
                sscanf(value, "%lf", &listen_port);
              else if (!strcasecmp(field, "sounddriver"))
                sscanf(value, "%lf", &sound_driver);
              else if (!strcasecmp(field, "playmusic"))
                sscanf(value, "%lf", &play_music);
              else if (!strcasecmp(field, "fullscreen"))
                sscanf(value, "%lf", &full_screen);
              else if (!strcasecmp(field, "scoreboard"))
                sscanf(value, "%d", &show_scoreboard);
            }    // end of found field=value line

        }     // end of read a line properly
    }     // end of while not done
  if (! sound_bookmark)
    sound = sound_bookmark;

  if (width_override)
    screenWidth = width_override;
  if (height_override)
    screenHeight = height_override;

  halfWidth = screenWidth / 2;
  halfHeight = screenHeight / 2;

  menuBeginY = (screenHeight - 400) / 2;
  if (menuBeginY < 0) menuBeginY = 0;
  menuEndY = screenHeight - menuBeginY;

  if (skipComputerPlay > SKIP_HUMANS_DEAD)
    skipComputerPlay = SKIP_HUMANS_DEAD;

  return TRUE;
}





void GLOBALDATA::initialise ()
{
  numTanks = 0;
}

void GLOBALDATA::addPlayer (PLAYER *player)
{
  if (numPlayers < MAXPLAYERS)
    {
      players[numPlayers] = player;
      numPlayers++;
      if ((int)player->type == HUMAN_PLAYER)
        {
          numHumanPlayers++;
          computerPlayersOnly = FALSE;
        }
    }
}

void GLOBALDATA::removePlayer (PLAYER *player)
{
  int fromCount = 0;
  int toCount = -1;

  if ((int)player->type == HUMAN_PLAYER)
    {
      numHumanPlayers--;
      if (numHumanPlayers == 0)
        {
          computerPlayersOnly = TRUE;
        }
    }

  while (fromCount < numPlayers)
    {
      if (player != players[fromCount])
        {
          if ((toCount >= 0) && (fromCount > toCount))
            {
              players[toCount]    = players[fromCount];
              players[fromCount]  = NULL;
              toCount++;
            }
        }
      else
        // Position found,1G now move the remaining players down!
        toCount = fromCount;
      fromCount++;
    }
  numPlayers--;
}

PLAYER *GLOBALDATA::getNextPlayer (int *playerCount)
{
  (*playerCount)++;
  if (*playerCount >= numPlayers)
    *playerCount = 0;
  return (players[*playerCount]);
}

PLAYER *GLOBALDATA::createNewPlayer (ENVIRONMENT *env)
{
  PLAYER **reallocatedPlayers;
  PLAYER *player;

  reallocatedPlayers = (PLAYER**)realloc (allPlayers, sizeof (PLAYER*) * (numPermanentPlayers + 1));
  if (reallocatedPlayers != NULL)
    {
      allPlayers = reallocatedPlayers;
    }
  else
    {
      perror ( (char *)"atanks.cc: Failed allocating memory for reallocatedPlayers in GLOBALDATA::createNewPlayer");
      // exit (1);
    }
  player = new PLAYER (this, env);
  if (!player)
    {
      perror ( (char *)"globaldata.cc: Failed allocating memory for player in GLOBALDATA::createNewPlayer");
      // exit (1);
    }
  allPlayers[numPermanentPlayers] = player;
  numPermanentPlayers++;

  return (player);
}

void GLOBALDATA::destroyPlayer (PLAYER *player)
{
  int fromCount = 0;
  int toCount = 0;

  for (; fromCount < numPermanentPlayers; fromCount++)
    {
      if (allPlayers[fromCount] != player)
        {
          allPlayers[toCount] = allPlayers[fromCount];
          toCount++;
        }
    }
  numPermanentPlayers--;
}



// This function returns the path to the
// config directory used by Atanks
char *GLOBALDATA::Get_Config_Path()
{
  char *my_config_dir;
  char *homedir;

  // figure out file name
  homedir = getenv(HOME_DIR);
  if (! homedir)
    homedir = (char *)".";
  my_config_dir = (char *) calloc( strlen(homedir) + 24,
                                   sizeof(char) );
  if (! my_config_dir)
    return NULL;

  sprintf (my_config_dir, "%s/.atanks", homedir);
  return my_config_dir;

}



// This function checks to see if one full second has passed since the
// last time the function was called.
// The function returns true if time has passed. The function
// returns false if time hasn't passed or it was unable to tell
// how much time has passed.
bool GLOBALDATA::Check_Time_Changed()
{
  static time_t last_second = 0;
  time_t current_second;

  current_second = time(NULL);
  if ( current_second == last_second )
    return false;

  // time has changed
  last_second = current_second;
  return true;
}



/*
 * This function Loads a music file (if there is one available.
 * A pointer to the music file is returned. If no music can
 * be found, then NULL is returned.
*/
SAMPLE *GLOBALDATA::Load_Background_Music()
{
    SAMPLE *my_sample = NULL;;
    struct dirent *folder_entry;

    // see if we should bother
    if (! play_music)
       return NULL;

    // see if we have the music folder open
    if (! music_dir)
    {
        char *buffer = (char *) calloc( strlen(configDir) + 32, sizeof(char) );
        if (! buffer)
           return NULL;

        sprintf(buffer, "%s/music", configDir);
        music_dir = opendir(buffer);
        free(buffer);
        if (! music_dir)
           return NULL;
    }

    // at this point we should have an open music folder
    // the music folder is closed by global's deconstructor
    // search for files ending in .wav
    folder_entry = readdir(music_dir);
    while ( (folder_entry) && (! my_sample) )
    {
        // we have something, see if it is a wav file
        if ( strstr(folder_entry->d_name, ".wav") )
        {
            char *filename = (char *) calloc( strlen(configDir) + strlen(folder_entry->d_name) + 64, sizeof(char));
            if (filename)
            {
               sprintf(filename, "%s/music/%s", configDir, folder_entry->d_name);
               my_sample = load_sample(filename);
               free(filename);
            }
        }
        if (! my_sample)
           folder_entry = readdir(music_dir);
    }

    if (! folder_entry)  // hit end of folder
    {
       closedir(music_dir);
       music_dir = NULL;
    }

    return my_sample;
}




/*
 * This function sets all variables, which get written to the
 * config file, back to their defaults.
 * -- Jesse
 *  */
void GLOBALDATA::Reset_Options()
{
  ditherGradients = 1;
  detailedLandscape = 0;
  detailedSky = 0;
  interest = 1.25;
  scoreRoundWinBonus = 10000;
  scoreHitUnit = 75;
  scoreUnitDestroyBonus = 5000;
  scoreUnitSelfDestroy = 0;
  sellpercent = 0.80;
  startmoney = 15000;
  turntype = TURN_RANDOM;
  skipComputerPlay = SKIP_HUMANS_DEAD;
  sound = 1.0;
  screenWidth = DEFAULT_SCREEN_WIDTH;
  screenHeight = DEFAULT_SCREEN_HEIGHT;
  os_mouse = 1.0;
  language = LANGUAGE_ENGLISH;
  colour_theme = COLOUR_THEME_CRISPY;
  frames_per_second = FRAMES_PER_SECOND;
  violent_death = FALSE;
  max_fire_time = 0.0;
  divide_money = 0.0;
  check_for_updates = 1.0;
  enable_network = 0.0;
  #ifdef NETWORK
  listen_port = DEFAULT_LISTEN_PORT;
  #endif
  play_music = 1.0;
} 




/*
 * This function loads all sounds from the data folder and saves them
 * in an array.
 * The function returns TRUE on success or FALSE if an error happens.
*/
int GLOBALDATA::Load_Sounds()
{
   int file_count = 0, array_size = 10;
   char *file_name;
   FILE *my_file;
   SAMPLE *temp_sample;

   file_name = (char *) calloc( strlen(dataDir) + 128, sizeof(char) );
   if (! file_name)
     return FALSE;

   // allocate space for sound samples
   sounds = (SAMPLE **) calloc(10, sizeof(SAMPLE *) );
   if (! sounds)
   {
       free(file_name);
       printf("Unable to create sound array.\n");
       return FALSE;
   }

   // read from directory
   sprintf(file_name, "%s/sound/%d.wav", dataDir, file_count);
   my_file = fopen(file_name, "r");
   while ( (my_file) && (sounds) )
   {
       fclose(my_file);
       temp_sample = load_sample(file_name);
       if (! temp_sample )
         printf("An error occured loading sound file %s\n", file_name);
       sounds[file_count] = temp_sample;
       file_count++; 
   
       // make sure we have enough memory for more samples
       if (file_count >= array_size)
       {
            array_size += 10;
            sounds = (SAMPLE**) realloc(sounds, sizeof(SAMPLE*) * (array_size + 1));
            if (! sounds)
              printf("We just ran out of memory loading sound files.\n");
            else
            {
              // zero out new memory pointers
              int counter;
              for (counter = file_count; counter <= array_size; counter++)
                 sounds[counter] = NULL;
            }
       }
       sprintf(file_name, "%s/sound/%d.wav", dataDir, file_count);
       my_file = fopen(file_name, "r");
   }

   free(file_name);
   return TRUE;
}



/*
 * This function loads all the bitmaps needed by th game.
 * Bitmaps are found in a series of sub-folders under the
 * dataDir. The function returns TRUE on success and FALSE
 * if an error occures.
*/
int GLOBALDATA::Load_Bitmaps()
{
   int file_group = 0;
   char *file_name;
   char sub_folder[64];
   BITMAP *new_bitmap, **bitmap_array;

   file_name = (char *) calloc( strlen(dataDir) + 128, sizeof(char) );
   if (!file_name)
      return FALSE;

   while (file_group < 7)
   {
       int array_size = 10;
       int file_count = 0;

     // set the folder we're looking at
     switch (file_group)
     {
        case 0: strcpy(sub_folder, "title"); break;
        case 1: strcpy(sub_folder, "button"); break;
        case 2: strcpy(sub_folder, "misc"); break;
        case 3: strcpy(sub_folder, "missile"); break;
        case 4: strcpy(sub_folder, "stock"); break;
        case 5: strcpy(sub_folder, "tank"); break;
        case 6: strcpy(sub_folder, "tankgun"); break;
     }

     // set up empty array
     bitmap_array = (BITMAP **) calloc(10, sizeof(BITMAP *) );
     if (! bitmap_array)
     {
         printf("Ran out of memory, loading bitmaps.\n");
         free(file_name);
         return FALSE;
     }

     // search for files
     sprintf(file_name, "%s/%s/%d.bmp", dataDir, sub_folder, file_count);
     FILE *my_file = fopen(file_name, "r");
     while ( (my_file) && (bitmap_array) )
     {
         fclose(my_file);
         new_bitmap = load_bitmap(file_name, NULL);
         if (! new_bitmap)
           printf("An error occured loading bitmap %s\n", file_name);
         bitmap_array[file_count] = new_bitmap;
         file_count++;

         // make sure array is large enough
         if ( file_count >= array_size)
         {
            array_size += 10;
            bitmap_array = (BITMAP **) realloc(bitmap_array, sizeof(BITMAP *) * (array_size + 1) );
            if (! bitmap_array)
               printf("Unable to increase array size while loading bitmaps.\n");
            else
            {
               // clear memory
               int count;
               for (count = file_count; count <= array_size; count++)
                  bitmap_array[count] = NULL;
            }
         }

         // get next file
         sprintf(file_name, "%s/%s/%d.bmp", dataDir, sub_folder, file_count);
         my_file = fopen(file_name, "r");
     }
 
     // save the new array
     switch (file_group)
     {
        case 0: title = bitmap_array; break;
        case 1: button = bitmap_array; break;
        case 2: misc = bitmap_array; break;
        case 3: missile = bitmap_array; break;
        case 4: stock = bitmap_array; break;
        case 5: tank = bitmap_array; break;
        case 6: tankgun = bitmap_array; break;
     }

     file_group++;
   }

   free(file_name);
   return TRUE;
}



// This file loads in extra fonts the game requires.
// Fonts should be stored in the datafolder. On
// success the function returns TRUE. When an
// error occures, it returns FALSE.
int GLOBALDATA::Load_Fonts()
{
   char *filename;

   filename = (char *) calloc( strlen(dataDir) + 32, sizeof(char));
   if (!filename)
     return FALSE;

   sprintf(filename, "%s/unicode.dat", dataDir);
   unicode = load_font(filename, NULL, NULL);
   if (! unicode)
      printf("Unable to load font %s\n", filename);
   free(filename);

   if (unicode)
   {
      Change_Font();
      return TRUE;
   }
   else return FALSE;  
}


// This function selects the font to use. This should be called
// right after a language change.
void GLOBALDATA::Change_Font()
{
    // FONT *temp_font = font;

    if ( (language == LANGUAGE_RUSSIAN) || (language == LANGUAGE_GERMAN) )
       font = unicode;
    else
       font = regular_font;

    // if (temp_font != font)     // font has changed
    // {
       Load_Weapons_Text(this);
       Load_Text_Files();
    // }
}



void GLOBALDATA::Update_Player_Menu()
{
   int index;

   for (index = 0; index < numPermanentPlayers; index++)
   {
       if (allPlayers[index])
          allPlayers[index]->initMenuDesc();
   }
}




// This function loads all needed text files, based on
// language, into memory. If a previous text was loaded, it is
// removed from memory first.
int GLOBALDATA::Load_Text_Files()
{
    char *filename;
    char suffix[32];

    filename = (char *) calloc( strlen(dataDir) + 64, sizeof(char) );
    if (! filename)
       return FALSE;

    if (war_quotes) delete war_quotes;
    if (instructions) delete instructions;
    if (gloat) delete gloat;
    if (revenge) delete revenge;
    if (retaliation) delete retaliation;
    if (suicide) delete suicide;
    if (kamikaze) delete kamikaze;
    if (ingame) delete ingame;

    if (language == LANGUAGE_RUSSIAN)
       sprintf(filename, "%s/text/war_quotes_ru.txt", dataDir);
    else if (language == LANGUAGE_SPANISH)
       sprintf(filename, "%s/text/war_quotes_ES.txt", dataDir);
    else
       sprintf(filename, "%s/text/war_quotes.txt", dataDir);
    war_quotes = new TEXTBLOCK(filename);

     if (language == LANGUAGE_PORTUGUESE)
        strcpy(suffix, ".pt_BR.txt");
     else if (language == LANGUAGE_FRENCH)
        strcpy(suffix, "_fr.txt");
     else if (language == LANGUAGE_GERMAN)
        strcpy(suffix, "_de.txt");
     else if (language == LANGUAGE_SLOVAK)
        strcpy(suffix, "_sk.txt");
     else if (language == LANGUAGE_RUSSIAN)
        strcpy(suffix, "_ru.txt");
     else if (language == LANGUAGE_SPANISH)
        strcpy(suffix, "_ES.txt");
     else if (language == LANGUAGE_ITALIAN)
        strcpy(suffix, "_it.txt");
     else
        strcpy(suffix, ".txt");       // default to english

    sprintf(filename, "%s/text/instr%s", dataDir, suffix);
    instructions = new TEXTBLOCK(filename);

    sprintf(filename, "%s/text/gloat%s", dataDir, suffix);
    gloat = new TEXTBLOCK(filename);
    sprintf(filename, "%s/text/revenge%s", dataDir, suffix);
    revenge = new TEXTBLOCK(filename);
    sprintf(filename, "%s/text/retaliation%s", dataDir, suffix);
    retaliation = new TEXTBLOCK(filename);
    sprintf(filename, "%s/text/suicide%s", dataDir, suffix);
    suicide = new TEXTBLOCK(filename);
    sprintf(filename, "%s/text/kamikaze%s", dataDir, suffix);
    kamikaze = new TEXTBLOCK(filename);
    sprintf(filename, "%s/text/ingame%s", dataDir, suffix);
    ingame = new TEXTBLOCK(filename);

    free(filename);
    return TRUE;
}


#ifdef NETWORK
// This function sends a message to all connected game clients.
// Returns TRUE on success or FALSE if the message could not be sent
int GLOBALDATA::Send_To_Clients(char *message)
{
   int index;
   int message_length;

   if (! message) return FALSE;
   message_length = strlen(message);
   for (index = 0; index < numPlayers; index++)
   {
       if ( (players[index]) && (players[index]->type == NETWORK_CLIENT) )
          write(players[index]->server_socket, message, message_length);
   }     // done all players
   return TRUE;
}
#endif



// This function tries to figure out where the dataDir is. It
// first checks the current working directory, ".". If the
// proper files are not found, then we try the defined value of
// DATA_DIR.
// On success, TRUE is returned. If no usable directory is
// found, then FALSE is returned.
int GLOBALDATA::Find_Data_Dir()
{
     char *current_dir = NULL;
     FILE *my_phile;

     current_dir = (char *) calloc( strlen(DATA_DIR) + 32, sizeof(char));
     if (! current_dir)
        return FALSE;

     // try current dir
     strcpy(current_dir, "./unicode.dat");   // local dir
     my_phile = fopen(current_dir, "r");
     if (my_phile)
     {
         fclose(my_phile);
         free(current_dir);
         dataDir = ".";
         return TRUE;
     }

    // try system dir
    sprintf(current_dir, "%s/unicode.dat", DATA_DIR);
    my_phile = fopen(current_dir, "r");
    if (my_phile)
    {
        fclose(my_phile);
        free(current_dir);
        dataDir = DATA_DIR;
        return TRUE;
    }

    dataDir = DATA_DIR;    // fall back
    free(current_dir);
    return FALSE;
}



/*
Find the max velocity of a missile
*/
double GLOBALDATA::Calc_Max_Velocity()
{
   dMaxVelocity = (double) MAX_POWER * (100.0 / (double) frames_per_second) / 100.0;
   return dMaxVelocity; 
}



/* See how many humans and networked players we have */
int GLOBALDATA::Count_Humans()
{
     int count;
     int humans = 0;

     for (count = 0; count < numPlayers; count++)
     {
        if (players[count]->type == HUMAN_PLAYER)
           humans++;
        else if (players[count]->type == NETWORK_CLIENT)
           humans++;
     }

     return humans;
}


/*
This function checks to see if there is a single player left standing.
If there is, that player's index is returned. If there is no winner, then
-1 is returned. If all tanks have been destroyed, then -2 is returned.
*/
int GLOBALDATA::Check_For_Winner()
{
    int index = 0, tank_count = 0;
    int last_alive = -1;
    
    while ( (index < numPlayers) && (tank_count < 2) )
    {
         if (players[index]->tank)
         {
            last_alive = index;
            tank_count++;
         }
         index++;
    }

    if ( (last_alive >= 0) && (tank_count == 1) )
       return last_alive;
    else if (tank_count > 1)
       return -1;
    else      // all dead
       return -2;
}


/*
This function gives credits, score and money to the winner(s).
*/
void GLOBALDATA::Credit_Winners(int winner)
{
   int team_members = 0;
   int index;

   if (winner < 0)    // no winner
      return;

   if (winner == JEDI_WIN)
   {
      for (index = 0; index < numPlayers; index++)
      {
          if (players[index]->team == TEAM_JEDI)
          {
             players[index]->score++;
             players[index]->won++;
             team_members++;
          }
      }

   }

   else if (winner == SITH_WIN)
   {
       for (index = 0; index < numPlayers; index++)
       {
          if (players[index]->team == TEAM_SITH)
          {
             players[index]->score++;
             players[index]->won++;
             team_members++;
          }
       }
   }
   else // credit single winner
   {
      players[winner]->score++;
      players[winner]->won++;
      players[winner]->money += (int) scoreRoundWinBonus; 
   }

   // team gets their money too
   if (team_members)
   {
       int team_bonus = (int) scoreRoundWinBonus / team_members;
       for (index = 0; index < numPlayers; index++)
       {
           if ( ((winner == JEDI_WIN) && (players[index]->team == TEAM_JEDI) ) ||
                ((winner == SITH_WIN) && (players[index]->team == TEAM_SITH) ) )
                players[index]->money += team_bonus;
       }
   }
}

