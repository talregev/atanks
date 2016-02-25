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
#  error "VERSION information is missing. Fix Makefile."
#endif

#ifdef GENTOO
#  ifndef DATA_DIR
#    define DATA_DIR "/usr/share/games/atanks"
#  endif // DATA_DIR
#endif // GENTOO

#ifndef BUFFER_SIZE
# define BUFFER_SIZE 256
#endif


/// Important: debug.h not only detects which OS/compiler this is,
/// it puts an important fix with allegro on windows in place.
/// Therefore it *must* stay before the block including winalleg.h!
#include "debug.h"


// The windows port does some crazy stuff with int*_t types.
#if defined(ATANKS_IS_WINDOWS)
#  include <cstdint>
#  ifndef ALLEGRO_HAVE_STDINT_H
#    define ALLEGRO_HAVE_STDINT_H 1
#  endif // ALLEGRO_HAVE_STDINT_H
#  if !defined(ATANKS_SRC_ATANKS_CPP)
#    define ALLEGRO_NO_MAGIC_MAIN
#  endif // Not called from atanks.cpp
#endif // Windows build system

#  include <allegro.h>

#if defined(ATANKS_IS_WINDOWS)
#  include <winalleg.h>
#endif // windows


// For visual studio some "workarounds" must be put in place
#if defined(ATANKS_IS_MSVC)
#  define PATH_MAX MAX_PATH
   // Needed for M_PIl to show up
#  if !defined(_USE_MATH_DEFINES)
#    define _USE_MATH_DEFINES 1
#  endif
#  include <cmath>
#else
#  include <unistd.h>
#  include <cmath>
#endif // Windows versus Linux


// Be sure M_PI and M_PIl are set:
#if !defined(M_PI)
#  define M_PI 3.14159265358979323846f
#endif
#if !defined(M_PIl)
#  define M_PIl 3.14159265358979323846L
#endif


#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <algorithm>


#include "globaltypes.h"


#define	DONE_IMAGE             11
#define	FAST_UP_ARROW_IMAGE    12
#define	UP_ARROW_IMAGE         13
#define	DOWN_ARROW_IMAGE       14
#define	FAST_DOWN_ARROW_IMAGE  15


#ifndef HAS_DIRENT
#  if defined(ATANKS_IS_MSVC)
#    include "extern/dirent.h"
#  else
#    include <dirent.h>
#  endif // Linux
#  define HAS_DIRENT 1
#endif // HAS_DIRENT


// Some more workarounds to compile using visual studio:
#if defined(ATANKS_IS_MSVC)
#  define snprintf         atanks_snprintf
#  define strncpy(d, s, c) strncpy_s(d, c+1, s, c)
#  define strncat(d, s, c) strncat_s(d, c+1, s, c)
#  define sscanf           sscanf_s
#  define access           _access
#  define F_OK             02
#  define R_OK             04
#  define W_OK             02
#  define strcasecmp       _stricmp
#  define strdup           _strdup
#  define unlink           _unlink
#  define mkdir            _mkdir
#endif

// Note: See winclock.h why this is necessary
/// REMOVE_VS12_WORKAROUND
#if defined(ATANKS_IS_MSVC) && !defined(ATANKS_IS_AT_LEAST_MSVC13)
#define USLEEP(microseconds_) Sleep(microseconds_ / 1000);
#define MSLEEP(milliseconds_) Sleep(milliseconds_);
#else
#define USLEEP(microseconds_) std::this_thread::sleep_for(std::chrono::microseconds(microseconds_));
#define MSLEEP(milliseconds_) std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds_));
#endif // VS12 workaround
#define LINUX_SLEEP     MSLEEP(10)
#define LINUX_REST      MSLEEP(40)


using std::cerr;
using std::cout;
using std::endl;
using std::string;


// place to save config and save games
#if defined(ATANKS_IS_WINDOWS)
#  define HOME_DIR "AppData"
#elif defined(ATANKS_IS_LINUX)
#  define HOME_DIR "HOME"
#endif // Windows versus Linux

#ifndef	DATA_DIR
#  define DATA_DIR "."
#endif

#define MAX_OVERSHOOT 10000


// The nex few are some math helpers that shorten things dramatically.
#define SIGN(x_arg)   ((x_arg) < 0  ? -1  : 1 )
#define SIGNd(x_arg)  ((x_arg) < 0. ? -1. : 1.)
#define ROUND(x_arg)  static_cast<int32_t>( (x_arg) + (SIGNd(x_arg) * .5))
#define ROUNDu(x_arg) static_cast<uint32_t>((x_arg) + .5)

#define FABSDISTANCE2(x1,y1,x2,y2) \
	std::sqrt(std::pow(static_cast<double>(x2) - static_cast<double>(x1), 2.) \
	        + std::pow(static_cast<double>(y2) - static_cast<double>(y1), 2.) )
#define FABSDISTANCE3(x1, y1, z1, x2, y2, z2) \
	std::sqrt(std::pow(static_cast<double>(x2) - static_cast<double>(x1), 2.) \
	        + std::pow(static_cast<double>(y2) - static_cast<double>(y1), 2.) \
	        + std::pow(static_cast<double>(z2) - static_cast<double>(z1), 2.) )

#define ABSDISTANCE2(x1,y1,x2,y2)       ROUNDu(FABSDISTANCE2(x1,y1,x2,y2))
#define ABSDISTANCE3(x1,y1,z1,x2,y2,z2) ROUNDu(FABSDISTANCE3(x1,y1,z1,x2,y2,z2))

#define DEG2RAD(degree_) (static_cast<double>(degree_) * M_PIl / 180. )
#define RAD2DEG(radian_) (static_cast<double>(radian_) * 180.  / M_PIl)


/** @brief show or hide the custom mouse cursor
  *
  * This macro can either hide the custom mouse cursor when @a where is set
  * to nullptr, or draw it on @a where, which then must be a pointer to a
  * BITMAP.
  *
  * This macro should be used to hide the custom mouse cursor before doing any
  * drawing and to place the mouse cursor on @a where once all other drawing is
  * done.
  *
  * If the OS mouse cursor is used, this macro does nothing.
  *
  * @param[in] where BITMAP pointer to draw the custom cursor on or nullptr to
  * hide the custom mouse cursor.
**/
#define SHOW_MOUSE(where) { \
	if (!env.osMouse) { \
		if (where != nullptr) unscare_mouse(); \
		else scare_mouse(); \
		show_mouse(where); \
		/* Make the neccessary updates */ \
		if (where != nullptr) { \
			global.make_update (mouse_x, mouse_y, env.misc[0]->w, env.misc[0]->h); \
			global.make_update (lx, ly, env.misc[0]->w, env.misc[0]->h); \
			lx = mouse_x; \
			ly = mouse_y; \
		} \
	} \
}

#define MAXPLAYERS    10
#define MAX_POWER   2000
#define MIN_POWER    100
#define MAX_ROUNDS 10000

#define MENUHEIGHT 40
#define BOXED_TOP 41 // This is the highest non-border pixel in boxed mode
#define	BALLISTICS 53
#define	BEAMWEAPONS 3
#define WEAPONS (BALLISTICS + BEAMWEAPONS)
#define ITEMS 24
#define THINGS (WEAPONS + ITEMS)
#define	NATURALS 6
#define DIRT_FRAGMENT -1
#define RADII 6
#define MAXRADIUS 200
#define BUTTONFRAMES 2

#define MENUBUTTONS 7
#define INGAMEBUTTONS 4
#define SPREAD 10
#define NAME_LEN 24
#define ADDRESS_LENGTH 16

#define WAIT_AT_END_OF_ROUND 1 // second (enough with the new live score board)

#define MAX_ITEM_NAME_LEN 127
#define MAX_ITEM_DESC_LEN 511
#define MAX_ITEMS_IN_STOCK 999999
#define MAX_MONEY_IN_WALLET 1000000000

// to make the theft bomb base steal easier to change, here
// is a useful define. Maybe, one day, we'll add it to the options?
#define THEFT_AMOUNT 5000

// Use these instead of the (most strict) defaults,
// But only where timing by memory fences do not matter.
#define ATOMIC_READ  std::memory_order_acquire
#define ATOMIC_WRITE std::memory_order_release


//turns
enum turnTypes
{
	TURN_HIGH = 0,
	TURN_LOW,
	TURN_RANDOM,
	TURN_SIMUL
};


struct POINT_t
{
	int32_t x = 0;
	int32_t y = 0;

	explicit
	POINT_t() { }
	POINT_t(int32_t x_, int32_t y_);
	POINT_t &operator=( const POINT_t &src );
};


struct BOX
{
	int32_t x = 0;
	int32_t y = 0;
	int32_t w = 0;
	int32_t h = 0;

	explicit
	BOX () { }
	BOX (int32_t x_, int32_t y_, int32_t w_, int32_t h_);
	BOX &operator=(const BOX  &src);
	BOX &operator=(const BOX &&src);

	void set(int32_t x_, int32_t y_, int32_t w_, int32_t h_);
};

// Make the BOX usage easier:
bool operator==(const BOX &lhs, const BOX &rhs);
bool operator!=(const BOX &lhs, const BOX &rhs);


struct gradient
{
	RGB color;
	float point;
};


class WEAPON
{
public:

	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	explicit WEAPON();


	/* -----------------------------------
	 * --- Public methods              ---
	 * -----------------------------------
	 */

	int32_t     getDelayDiv() const;
	const char* getDesc()     const;
	const char* getName()     const;

	void setDesc(const char* desc_);
	void setName(const char* name_);


	/* -----------------------------------
	 * --- Public members              ---
	 * -----------------------------------
	 */
	int32_t cost            = 0;  //!< $ :))
	int32_t amt             = 0;  //!< number of weapons in one buying package
	double  mass            = 0.;
	double  drag            = 0.;
	int32_t radius          = 0;  //!< of the explosion
	int32_t sound           = 0;
	int32_t etime           = 0;
	int32_t damage          = 0;  //!< damage power
	int32_t picpoint        = 0;  //!< which picture do we show in flight?
	int32_t spread          = 0;  //!< number of weapons in the shot
	int32_t delay           = 0;  //!< volleys etc.
	int32_t noimpact        = 0;
	int32_t techLevel       = 0;
	int32_t warhead         = 0;  //!< Is it a warhead?
	int32_t numSubmunitions = 0;  //!< Number of submunitions
	int32_t submunition     = 0;  //!< The next stage
	double  impartVelocity  = 0.; //!< Impart velocity 0.0-1.0 to subs
	int32_t divergence      = 0;  //!< Total angle for submunition spread
	double  spreadVariation = 0.; //!< Uniform or random distribution
	                              //!< 0-1.0 (0=uniform, 1.0=random)
	                              //!< divergence at centre of range
	double  launchSpeed     = 0.; //!< Speed given to submunitions
	double  speedVariation  = 0.; //!< Uniform or random speed
	                              //!< 0-1.0 (0=uniform, 1.0=random)
	                              //!< launchSpeed at centre of range
	int32_t countdown       = 0;  //!< Set the countdown to this
	double  countVariation  = 0.; //!< Uniform or random countdown
	                              //!< 0-1.0 (0=uniform, 1.0=random)
	                              //!< countdown at centre of range


private:


	/* -----------------------------------
	 * --- Private members             ---
	 * -----------------------------------
	 */

	char desc[MAX_ITEM_DESC_LEN + 1];
	char name[MAX_ITEM_NAME_LEN + 1];
};

#define	MAX_ITEMVALS	10
class ITEM
{
public:

	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	explicit ITEM();


	/* -----------------------------------
	 * --- Public methods              ---
	 * -----------------------------------
	 */

	const char* getDesc() const;
	const char* getName() const;

	void setDesc(const char* desc_);
	void setName(const char* name_);


	/* -----------------------------------
	 * --- Public members              ---
	 * -----------------------------------
	 */

	int32_t cost       = 0;
	int32_t amt        = 0;
	int32_t selectable = 0;
	int32_t techLevel  = 0;
	int32_t sound      = 0;
	double  vals[MAX_ITEMVALS];


private:


	/* -----------------------------------
	 * --- Private members             ---
	 * -----------------------------------
	 */

	char desc[MAX_ITEM_DESC_LEN + 1];
	char name[MAX_ITEM_NAME_LEN + 1];
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
	SELFD_TYPE = 0,
	SELFD_NUMBER
};


enum weaponType
{
	SML_MIS            =  0,
	MED_MIS            =  1,
	LRG_MIS            =  2,
	SML_NUKE           =  3,
	NUKE               =  4,
	DTH_HEAD           =  5,
	SML_SPREAD         =  6,
	MED_SPREAD         =  7,
	LRG_SPREAD         =  8,
	SUP_SPREAD         =  9,
	DTH_SPREAD         = 10,
	ARMAGEDDON         = 11,
	CHAIN_MISSILE      = 12,
	CHAIN_GUN          = 13,
	JACK_HAMMER        = 14,
	SHAPED_CHARGE      = 15,
	WIDE_BOY           = 16,
	CUTTER             = 17,
	SML_ROLLER         = 18,
	LRG_ROLLER         = 19,
	DTH_ROLLER         = 20,
	SMALL_MIRV         = 21,
	ARMOUR_PIERCING    = 22,
	CLUSTER            = 23,
	SUP_CLUSTER        = 24,
	FUNKY_BOMB         = 25,
	FUNKY_DEATH        = 26,
	FUNKY_BOMBLET      = 27,
	FUNKY_DEATHLET     = 28,
	BOMBLET            = 29,
	SUP_BOMBLET        = 30,
	BURROWER           = 31,
	PENETRATOR         = 32,
	SML_NAPALM         = 33,
	MED_NAPALM         = 34,
	LRG_NAPALM         = 35,
	NAPALM_JELLY       = 36,
	DRILLER            = 37,
	TREMOR             = 38,
	SHOCKWAVE          = 39,
	TECTONIC           = 40,
	RIOT_BOMB          = 41,
	HVY_RIOT_BOMB      = 42,
	RIOT_CHARGE        = 43,
	RIOT_BLAST         = 44,
	DIRT_BALL          = 45,
	LRG_DIRT_BALL      = 46,
	SUP_DIRT_BALL      = 47,
	SMALL_DIRT_SPREAD  = 48,
	CLUSTER_MIRV       = 49,
	PERCENT_BOMB       = 50,
	REDUCER            = 51,
	THEFT_BOMB         = 52, // Last ballistic (WEAPONS == 53)
	SML_LAZER          = 53,
	MED_LAZER          = 54,
	LRG_LAZER          = 55, // Last weapon (WEAPONS == 56)
	SML_METEOR         = 56,
	MED_METEOR         = 57,
	LRG_METEOR         = 58,
	SML_LIGHTNING      = 59,
	MED_LIGHTNING      = 60,
	LRG_LIGHTNING      = 61 // Last natural
};


#define LAST_EXPLOSIVE DRILLER

#define ITEM_NO_SHIELD   -1
enum itemType
{
	ITEM_TELEPORT             =  0, // 55 (weap_idx - WEAPONS)
	ITEM_SWAPPER              =  1, // 56
	ITEM_MASS_TELEPORT        =  2, // 57
	ITEM_FAN                  =  3, // 58
	ITEM_VENGEANCE            =  4, // 59
	ITEM_DYING_WRATH          =  5, // 60
	ITEM_FATAL_FURY           =  6, // 61
	ITEM_LGT_SHIELD           =  7,
	ITEM_MED_SHIELD           =  8,
	ITEM_HVY_SHIELD           =  9,
	ITEM_LGT_REPULSOR_SHIELD  = 10,
	ITEM_MED_REPULSOR_SHIELD  = 11,
	ITEM_HVY_REPULSOR_SHIELD  = 12,
	ITEM_ARMOUR               = 13,
	ITEM_PLASTEEL             = 14,
	ITEM_INTENSITY_AMP        = 15,
	ITEM_VIOLENT_FORCE        = 16,
	ITEM_SLICKP               = 17,
	ITEM_DIMPLEP              = 18,
	ITEM_PARACHUTE            = 19,
	ITEM_REPAIRKIT            = 20,
	ITEM_FUEL                 = 21, // 76
	ITEM_ROCKET               = 22, // 77
	ITEM_SDI                  = 23 // Last item
};

#define SHIELD_COUNT		6

//signals
#define	SIG_QUIT_GAME         -1
#define SIG_OK                 0
#define GLOBAL_COMMAND_QUIT   -1
#define GLOBAL_COMMAND_MENU    0
#define GLOBAL_COMMAND_OPTIONS 1
#define GLOBAL_COMMAND_PLAYERS 2
#define GLOBAL_COMMAND_CREDITS 3
#define	GLOBAL_COMMAND_HELP    4
#define GLOBAL_COMMAND_PLAY    5
#define GLOBAL_COMMAND_DEMO    6
#define GLOBAL_COMMAND_NETWORK 7


/** @enum eClasses
  * @brief class definitions of everything from virtual objects up
  *
  * The ordering here determines the order of the drawing.
**/
enum eClasses
{
	CLASS_MISSILE = 0,
	CLASS_BEAM,
	CLASS_TANK,
	CLASS_TELEPORT,
	CLASS_DECOR_DIRT,
	CLASS_DECOR_SMOKE,
	CLASS_EXPLOSION,
	CLASS_FLOATTEXT,
	CLASS_COUNT
};


#ifndef HAS_TANK
class TANK; // forwarding if not known
#endif // HAS_TANK

/// === Global functions used in several compilation units ====
void   drawMenuBackground(eBackgroundTypes backType, int32_t tOffset,
                          int32_t numItems);
double interpolate       (double x1, double x2, double i);
double Noise             (int x);
double Noise2D           (int x, int y);
double perlin1DPoint     (double amplitude, double scale, double xo,
                          double lambda, int octaves);
double perlin2DPoint     (double amplitude, double scale, double xo, double yo,
                          double lambda, int octaves);
void   quickChange       (bool clearerror);

#include "externs.h"

#endif

