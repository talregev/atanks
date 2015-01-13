#ifndef ENVIRONMENT_DEFINE
#define ENVIRONMENT_DEFINE

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

enum landscapeTypes
{
    LANDTYPE_RANDOM,
    LANDTYPE_CANYONS,
    LANDTYPE_MOUNTAINS,
    LANDTYPE_VALLEYS,
    LANDTYPE_HILLS,
    LANDTYPE_FOOTHILLS,
    LANDTYPE_PLAIN,
    LANDTYPE_NONE
};

enum landSlideTypes
{
    LANDSLIDE_NONE,       // gravity does not exist
    LANDSLIDE_TANK_ONLY,  // dirt falls, tank does not
    LANDSLIDE_INSTANT,    // dirt falls without you seeing it
    LANDSLIDE_GRAVITY,    // normal
    LANDSLIDE_CARTOON     // gravity is delayed
};

#ifndef MAX_GRAVITY_DELAY
#define GRAVITY_DELAY 200
#define MAX_GRAVITY_DELAY 3
#endif

enum wallTypes
{
    WALL_RUBBER,
    WALL_STEEL,
    WALL_SPRING,
    WALL_WRAP,
    WALL_RANDOM
};

// stages
#define STAGE_AIM 0
#define STAGE_FIRE 1
#define STAGE_ENDGAME 3


#define SPRING_CHANGE 1.25

// size of satellite laser
#define LASER_NONE 0.0
#define LASER_WEAK 1.0
#define LASER_STRONG 2.0
#define LASER_SUPER 3.0

#define _GET_R(x) ((x & 0xff0000) >> 16)
#define _GET_G(x) ((x & 0x00ff00) >> 8)
#define _GET_B(x) (x & 0x0000ff)

// max volume factor: means that the interval 0% -> 100% is splitted in 5
#define MAX_VOLUME_FACTOR 5

#include "tank.h"
//class TANK;
//class MISSILE;
//class FLOATTEXT;
//class GLOBALDATA;
//class VIRTUAL_OBJECT;
class ENVIRONMENT
{
public:
    GLOBALDATA *_global;
    TANK *order[MAXPLAYERS * 3];
    PLAYER *playerOrder[MAXPLAYERS * 3];
    VIRTUAL_OBJECT *objects[MAX_OBJECTS];
    int availableItems[THINGS];
    int numAvailable;
    int realm, am;
    double wind, lastwind;
    double windstrength, windvariation;
    double viscosity;
    double gravity;
    double weapontechLevel, itemtechLevel;
    double meteors;
    double lightning;
    double falling_dirt_balls;
    double fog;
    // int oldFogX, oldFogY;
    double landType;
    double landSlideType;
    double landSlideDelay;
    char *done;
    int *fp;
    int *h;
    int *surface;
    int *dropTo;
    double *velocity;
    double *dropIncr;
    double *height;
    BITMAP *db, *terrain, *sky;
    BITMAP *waiting_sky, *waiting_terrain;     // sky saved in background
#ifdef THREADS
    pthread_mutex_t* waiting_terrain_lock;
    pthread_mutex_t* waiting_sky_lock;
    void destroy_lock(pthread_mutex_t* lock);
    pthread_mutex_t* init_lock(pthread_mutex_t* lock);
    void lock(pthread_mutex_t* lock);
    void unlock(pthread_mutex_t* lock);
#endif
    BITMAP* get_waiting_sky();
    BITMAP* get_waiting_terrain();
    const gradient **my_sky_gradients, **my_land_gradients;
    int pclock, mouseclock;
    int stage;
    int combineUpdates;
    double wallType;
    double dBoxedMode; // Can be 0.0 = off, 1.0 = on, 2.0 = random
    double dFadingText; // 0.0 = off, 1.0 = on
    double dShadowedText; // 0.0 = off, 1.0 = on
    int current_wallType, wallColour;
    int time_to_fall;    // amount of time dirt will hover
    double satellite;
    int naturals_since_last_shot;
    double custom_background;
    char **bitmap_filenames;
    int number_of_bitmaps;
    int current_drawing_mode;
    int global_tank_index;
    int volume_factor;  // from 0 (no volume) to MAX_VOLUME_FACTOR

    ENVIRONMENT(GLOBALDATA *global);
    ~ENVIRONMENT();
    void initialise();
    void setGlobaldata(GLOBALDATA *global)
    {
        _global = global;
    }
    GLOBALDATA *Get_Globaldata()
    {
        return _global;
    }
    void generateAvailableItemsList();
    int isItemAvailable(int itemNum);
    int addObject(VIRTUAL_OBJECT *object);
    int removeObject(VIRTUAL_OBJECT *object);
    VIRTUAL_OBJECT *getNextOfClass(int classNum, int *index);
    void newRound();
    int ingamemenu();
    void make_fullUpdate();
    int getAvgBgColor(int, int, int, int, double, double);
    void do_updates();
    void replaceCanvas();
    void make_update(int x, int y, int w, int h);
    void make_bgupdate(int x, int y, int w, int h);

    int saveToFile_Text(FILE *file);
    int loadFromFile_Text(FILE *file);
    int loadFromFile(ifstream &ifsFile);

    void Reset_Options();
    void Set_Wall_Colour();
    bool Get_Boxed_Mode(GLOBALDATA *global);
    TANK *Get_Next_Tank(int *wrapped_around);
    void increaseVolume();
    void decreaseVolume();
    int scaleVolume(int vol) const;
};

#endif
