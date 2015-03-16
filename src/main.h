#ifndef MAIN_DEFINE
#define MAIN_DEFINE

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

#ifndef VERSION
#define VERSION "6.2"
#endif

#ifdef GENTOO
#ifndef DATA_DIR
#define DATA_DIR "/usr/share/games/atanks"
#endif
#endif

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 256
#endif

#ifndef _UNUSED
#define _UNUSED __attribute__((unused))
#endif // _UNUSED
#ifndef _WARNUNUSED
#define _WARNUNUSED __attribute__((warn_unused_result))
#endif // _WARNUNUSED

#include <allegro.h>
#ifdef WIN32
#include <winalleg.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "imagedefs.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cmath>

#ifdef LINUX
#include <unistd.h>
#endif /* LINUX */

#define GAME_SPEED 14000

#ifdef LINUX
#define LINUX_SLEEP usleep(10000)
#define LINUX_REST usleep(40000)
#define LINUX_DREAMLAND sleep(5)
#endif

#ifdef MACOSX
#define LINUX_SLEEP usleep(10000)
#define LINUX_REST usleep(40000)
#define LINUX_DREAMLAND sleep(5)
#endif

#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#define LINUX_SLEEP Sleep(10)
#define LINUX_REST Sleep(40)
#define LINUX_DREAMLAND Sleep(500)
#endif

using namespace std;

// place to save config and save games
#ifdef WIN32
#define HOME_DIR "AppData"
#endif
#ifdef LINUX
#define HOME_DIR "HOME"
#endif
#ifdef MACOSX
#define HOME_DIR "HOME"
#endif

#ifndef DATA_DIR
#define DATA_DIR "."
#endif
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define HALF_WIDTH (SCREEN_WIDTH/2)
#define HALF_HEIGHT (SCREEN_HEIGHT/2)
#define STUFF_BAR_WIDTH 400
#define STUFF_BAR_HEIGHT 35

#define GFILE_KEY 0x14233241

#define FRAMES_PER_SECOND 60
#ifndef PI
#define PI 3.1415926535897932384626433832795029L
#endif // PI
// #define PI 3.14
#define MAXUPDATES 256 // This one needs to be watched! If the display goes bye bye, reduce the value!
#define MAX_OVERSHOOT 10000
#define ABSDISTANCE(x1,y1,x2,y2) abs((int)sqrt((long double)pow(((long double)x2) - ((long double)x1), 2) + (long double)pow(((long double)y2) - ((long double)y1), 2)))
#define FABSDISTANCE(x1,y1,x2,y2) fabs(sqrt((long double)pow(((long double)x2) - ((long double)x1), 2) + (long double)pow(((long double)y2) - ((long double)y1), 2)))
#define MAXPLAYERS 10
#define MAX_POWER  2000
#define MAX_OBJECTS 400
#define MAX_ROUNDS 1000
#define TANKHEIGHT 20
#define TANKWIDTH 15
#define GUNLENGTH 16
#define MENUHEIGHT 30
#define SKIES 8
#define LANDS 8
#define ALL_SKIES 16
#define ALL_LANDS 16
#define BALLISTICS 52
#define BEAMWEAPONS 3
#define WEAPONS (BALLISTICS + BEAMWEAPONS)
#define ITEMS 24
#define THINGS (WEAPONS + ITEMS)
#define NATURALS 6
#define RADII 6
#define MAXRADIUS 200
#define BUTTONFRAMES 2
#define EXPLODEFRAMES 18
#define DISPERSEFRAMES 10
#define EXPLOSIONFRAMES (EXPLODEFRAMES + DISPERSEFRAMES)
#define TANKSAG 10
#define MENUBUTTONS 7
#define MISSILEFRAMES 1
#define ACHANGE 256/360
#define INGAMEBUTTONS 4
#define MAX_MISSILES 10
#define SPREAD 10
#define SHIELDS 6
#define WEAPONSOUNDS 4
#define NAME_LENGTH 24
#define ADDRESS_LENGTH 16

//Score coeficients

#define SCORE_DESTROY_UNIT_BONUS 5000
#define SCORE_UNIT_SELF_DESTROY 0
#define SCORE_HIT_UNIT 50
#define SCORE_SELF_HIT 0

//Wind
#define WIND_COEF 0.3
#define MAX_WIND 3

// bitmaps for tanks
#define NORMAL_TANK 0
#define CLASSIC_TANK 1     // really 8 in the .dat file
#define BIGGREY_TANK 2
#define T34_TANK 3
#define HEAVY_TANK 4
#define FUTURE_TANK 5
#define UFO_TANK 6
#define SPIDER_TANK 7
#define BIGFOOT_TANK 8
#define MINI_TANK 9

// background images
#define BACKGROUND_CIRCLE 0
#define BACKGROUND_LINE 1
#define BACKGROUND_BLANK 2

#ifdef NEW_GAMELOOP
#define WAIT_AT_END_OF_ROUND 300
#else
#define WAIT_AT_END_OF_ROUND 100
#endif

// time between volly shots
#define VOLLY_DELAY 50

// defines for teams
#define TEAM_SITH 0
#define TEAM_NEUTRAL 1
#define TEAM_JEDI 2

//turns
enum turnTypes
{
    TURN_HIGH, TURN_LOW, TURN_RANDOM, TURN_SIMUL
};

class BOX
{
public:
    int x, y, w, h;
};

typedef struct gradient_struct
{
    RGB color;
    float point;
} gradient;

class WEAPON
{
public:
    char name[128]; //name of weapon
    char description[512];
    int cost; //$ :))
    int amt; //number of weapons in one buying package
    double mass;
    double drag;
    int radius; // of the explosion
    int sound;
    int etime;
    int damage; //damage power
    int eframes;
    int picpoint; // which picture do we show in flight?
    int spread; // number of weapons in the shot
    int delay; // volleys etc.
    int noimpact;
    int techLevel;
    int warhead; // Is it a warhead?
    int numSubmunitions; // Number of submunitions
    int submunition; // The next stage
    double impartVelocity; // Impart velocity 0.0-1.0 to subs
    int divergence; // Total angle for submunition spread
    double spreadVariation; // Uniform or random distribution
    //   0-1.0 (0=uniform, 1.0=random)
    //   divergence at centre of range
    double launchSpeed; // Speed given to submunitions
    double speedVariation; // Uniform or random speed
    //   0-1.0 (0=uniform, 1.0=random)
    //   launchSpeed at centre of range
    int countdown; // Set the countdown to this
    double countVariation; // Uniform or random countdown
    //   0-1.0 (0=uniform, 1.0=random)
    //   countdown at centre of range
};

#define MAX_ITEMVALS 10
class ITEM
{
public:
    char name[128];
    char description[256];
    int cost;
    int amt;
    int selectable;
    int techLevel;
    int sound;
    double vals[MAX_ITEMVALS];
};

enum shieldVals
{
    SHIELD_ENERGY,
    SHIELD_REPULSION,
    SHIELD_RED,
    SHIELD_GREEN,
    SHIELD_BLUE,
    SHIELD_THICKNESS
};

enum selfDestructVals
{
    SELFD_TYPE,
    SELFD_NUMBER
};

enum weaponType
{
    SML_MIS, MED_MIS, LRG_MIS, SML_NUKE, NUKE, DTH_HEAD,
    SML_SPREAD, MED_SPREAD, LRG_SPREAD, SUP_SPREAD, DTH_SPREAD, ARMAGEDDON,
    CHAIN_MISSILE, CHAIN_GUN, JACK_HAMMER,
    SHAPED_CHARGE, WIDE_BOY, CUTTER,
    SML_ROLLER, LRG_ROLLER, DTH_ROLLER,
    SMALL_MIRV,
    ARMOUR_PIERCING,
    CLUSTER, SUP_CLUSTER,
    FUNKY_BOMB, FUNKY_DEATH,
    FUNKY_BOMBLET, FUNKY_DEATHLET,
    BOMBLET, SUP_BOMBLET,
    BURROWER, PENETRATOR,
    SML_NAPALM, MED_NAPALM, LRG_NAPALM,
    NAPALM_JELLY,
    DRILLER,
    TREMOR, SHOCKWAVE, TECTONIC,
    RIOT_BOMB, HVY_RIOT_BOMB,
    RIOT_CHARGE, RIOT_BLAST,
    DIRT_BALL, LRG_DIRT_BALL, SUP_DIRT_BALL,
    SMALL_DIRT_SPREAD,
    CLUSTER_MIRV,
    PERCENT_BOMB,
    REDUCER,
    SML_LAZER, MED_LAZER, LRG_LAZER,
    SML_METEOR, MED_METEOR, LRG_METEOR,
    SML_LIGHTNING, MED_LIGHTNING, LRG_LIGHTNING
};

// #define LAST_EXPLOSIVE NAPALM_JELLY
#define LAST_EXPLOSIVE DRILLER

#define ITEM_NO_SHIELD -1
enum itemType
{
    ITEM_TELEPORT,
    ITEM_SWAPPER,
    ITEM_MASS_TELEPORT,
    ITEM_FAN,
    ITEM_VENGEANCE,
    ITEM_DYING_WRATH,
    ITEM_FATAL_FURY,
    ITEM_LGT_SHIELD,
    ITEM_MED_SHIELD,
    ITEM_HVY_SHIELD,
    ITEM_LGT_REPULSOR_SHIELD,
    ITEM_MED_REPULSOR_SHIELD,
    ITEM_HVY_REPULSOR_SHIELD,
    ITEM_ARMOUR,
    ITEM_PLASTEEL,
    ITEM_INTENSITY_AMP,
    ITEM_VIOLENT_FORCE,
    ITEM_SLICKP,
    ITEM_DIMPLEP,
    ITEM_PARACHUTE,
    ITEM_REPAIRKIT,
    ITEM_FUEL,
    ITEM_ROCKET,
    ITEM_SDI
};

#define SHIELD_COUNT 6

//signals
#define SIG_QUIT_GAME -1
#define SIG_OK 0
#define GLOBAL_COMMAND_QUIT -1
#define GLOBAL_COMMAND_MENU 0
#define GLOBAL_COMMAND_OPTIONS 1
#define GLOBAL_COMMAND_PLAYERS 2
#define GLOBAL_COMMAND_CREDITS 3
#define GLOBAL_COMMAND_HELP 4
#define GLOBAL_COMMAND_PLAY 5
#define GLOBAL_COMMAND_DEMO 6
#define GLOBAL_COMMAND_NETWORK 7

// Classes
#define ANY_CLASS -1
#define VIRTUAL_OBJECT_CLASS 0
#define FLOATTEXT_CLASS 1
#define PHYSICAL_OBJECT_CLASS 2
#define MISSILE_CLASS 3
#define TANK_CLASS 4
#define EXPLOSION_CLASS 5
#define TELEPORT_CLASS 6
#define BEAM_CLASS 7
#define DECOR_CLASS 8

typedef struct
{
    // DATAFILE *M, *T, *TITLE, *S, *E, *B, *L,
    // *TG, *MI, *STOCK_IMAGE;

    BITMAP *sky_gradient_strips[ALL_SKIES];
    BITMAP *land_gradient_strips[ALL_LANDS];
    // BITMAP *circle_gradient_strip;
    BITMAP *stuff_bar_gradient_strip;
    BITMAP *topbar_gradient_strip;
    BITMAP *explosion_gradient_strip;
    BITMAP *stuff_bar[2];
    BITMAP *stuff_icon_base;
    // BITMAP *circlesBG;
    BITMAP *topbar;
    BITMAP *explosions[EXPLOSIONFRAMES];
    BITMAP *flameFront[EXPLOSIONFRAMES];
} gfxDataStruct;

class GLOBALDATA;
class ENVIRONMENT;
int drawFracture(GLOBALDATA *global, ENVIRONMENT *env, BITMAP *dest, BOX *updateArea, int x, int y, int angle, int width, int segmentLength, int maxRecurse, int recurseDepth);
int setSlideColumnDimensions(GLOBALDATA *global, ENVIRONMENT *env, int x, bool reset);
double Noise(int x);
double Noise2D(int x, int y);
double interpolate(double x1, double x2, double i);
double perlin1DPoint(double amplitude, double scale, double xo, double lambda, int octaves);
double perlin2DPoint(double amplitude, double scale, double xo, double yo, double lambda, int octaves);

void quickChange(GLOBALDATA *global, BITMAP *target) ;
void change(GLOBALDATA *global, BITMAP *target);

int checkPixelsBetweenTwoPoints(GLOBALDATA *global, ENVIRONMENT *env, double *startX, double *startY, double endX, double endY);
long int calcTotalPotentialDamage(int weapNum);
long int calcTotalEffectiveDamage(int weapNum);

void close_button_handler(void);

void doLaunch(GLOBALDATA *gd, ENVIRONMENT *env);

#define GENSKY_DETAILED 1
#define GENSKY_DITHERGRAD 2
void generate_sky(GLOBALDATA *global, BITMAP* bmp, const gradient* grad, int flags);

// load all game settings
bool Load_Game_Settings(GLOBALDATA *global, ENVIRONMENT *env, char *text_file) _WARNUNUSED;
bool loadPlayers(GLOBALDATA *global, ENVIRONMENT *env, ifstream &ifsFile) _WARNUNUSED;
// save all game settings
bool Save_Game_Settings(GLOBALDATA *global, ENVIRONMENT *env, char *text_file, bool bIsSaveGame = false) _WARNUNUSED;
bool savePlayers(GLOBALDATA *global, ofstream &ofsFile) _WARNUNUSED;
int Save_Game_Settings_Text(GLOBALDATA *global, ENVIRONMENT *env, char *path_to_file);

// These defines are needed to identify parts of the binary save files.
#define FILEPART_ENVIRON "[Environment]"
#define FILEPART_GLOBALS "[Global]"
#define FILEPART_PLAYERS "[Players]"
#define FILEPART_ENDSECT "[EndSection]"
#define FILEPART_ENDPLYR "[EndPlayer]"
#define FILEPART_ENDFILE "[EndFile]"

// methods to handle loading and saving of data

/*  --- take data out of filestream -- */
bool popColo(int &aData, ifstream &ifsFile) _WARNUNUSED;
bool popData(int &aData, ifstream &ifsFile) _WARNUNUSED;
bool popData(double &aData, ifstream &ifsFile) _WARNUNUSED;
bool popData(char * aArea, bool &aIsData, ifstream &ifsFile) _WARNUNUSED;
//bool popData(const char  * aArea, int    &aData, ifstream &ifsFile) _WARNUNUSED;
//bool popData(const char  * aArea, double &aData, ifstream &ifsFile) _WARNUNUSED;
//bool popData(char * aArea, double &aData, ifstream &ifsFile) _WARNUNUSED;

/*  --- put data into filestream -- */
//bool pushData(const int aData, ofstream &ofsFile) _WARNUNUSED;
//bool pushData(const double aData, ofstream &ofsFile) _WARNUNUSED;
bool pushColo(const char * aArea, const int aData, ofstream &ofsFile) _WARNUNUSED;
bool pushData(const char * aData, ofstream &ofsFile) _WARNUNUSED;
bool pushData(const char * aArea, const int aData, ofstream &ofsFile) _WARNUNUSED;
//bool pushName(const char  * aData, ofstream &ofsFile) _WARNUNUSED; // Special function for name saving
bool pushData(const char * aArea, const double aData, ofstream &ofsFile) _WARNUNUSED;

/*  --- seek specific data within filestream -- */
//bool seekData(const int aData, ifstream &ifsFile) _WARNUNUSED;
//bool seekData(const char * aData, ifstream &ifsFile) _WARNUNUSED;

// handle changes to global settings
int Change_Settings(double old_mouse, double old_sound, double new_mouse, double new_sound, void *mouse_image);

int *Sort_Scores(GLOBALDATA *global);
int Game_Client(GLOBALDATA *global, ENVIRONMENT *env, int socket_number);

#include "externs.h"

#endif
