#ifndef PLAYER_HEADER_
#define PLAYER_HEADER_

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
#include "menu.h"


enum playerType
{
  HUMAN_PLAYER,
  USELESS_PLAYER,
  GUESSER_PLAYER,
  RANGEFINDER_PLAYER,
  TARGETTER_PLAYER,
  DEADLY_PLAYER,
  LAST_PLAYER_TYPE,
  PART_TIME_BOT,         // normally a human, but acting as a deadly computer
  VERY_PART_TIME_BOT,    // just fires one shot
  NETWORK_CLIENT
};
//player weapon preference type
//ALWAYS_PREF - only choose weapon preferences once on player creation
//PERPLAY_PREF - choose weapon preferences once per game
enum playerPrefType
{
  PERPLAY_PREF,
  ALWAYS_PREF
};
enum turnStages
{
  SELECT_WEAPON,
  SELECT_TARGET,
  CALCULATE_ATTACK,
  AIM_WEAPON,
  FIRE_WEAPON
};

#define TANK_TYPES 9

#define	NUM_ROULETTE_SLOTS (THINGS * 100)
#define	MAX_WEAP_PROBABILITY	10000
#define BURIED_LEVEL 135

#define NET_COMMAND_SIZE 64
// if we do not get a command after this amount of seconds,
// turn control over to the computer for a moment
#define NET_DELAY 1000
#define NET_DELAY_SHORT 500


// values the control functions return ot the main game loop
#define CONTROL_PRESSED 2   // some key pressed, but not to shoot
#define CONTROL_FIRE 1
#define CONTROL_QUIT -1
#define CONTROL_SKIP -2


class TANK;
class PLAYER
  {

    char	_name[NAME_LENGTH];
    int	_currTank;
    GLOBALDATA *_global;
    ENVIRONMENT *_env;
    char	_turnStage;
    TANK	*_target;
    TANK  *_oldTarget;
    int	_targetAngle;
    int	_targetPower;
    int	_overshoot;
    int	_weaponPreference[THINGS];
    int	_rouletteWheel[NUM_ROULETTE_SLOTS];
    int	_targetX;
    int	_targetY;
    int iBoostItemsBought;
    int iTargettingRound;
    int computerSelectPreBuyItem (int aMaxBoostValue = 0);
    TANK * computerSelectTarget (int aPreferredWeapon, bool aRotationMode = false);
    char * selectKamikazePhrase();
    char * selectRetaliationPhrase();
    int getBlastValue (TANK * aTarget, int aDamage, int aWeapon, double aDamageMod = 1.0);
    int getUnburyingTool();
    int adjustOvershoot(int &aOvershoot, double aReachedX, double aReachedY);
    int getAdjustedTargetX(TANK * aTarget = NULL);
    int calculateAttack(TANK *aTarget = NULL);
    void calculateAttackValues(int &aDistanceX, int aDistanceY, int &aRawAngle, int &aAdjAngle, int &aPower, bool aAllowFlip);
    int	traceShellTrajectory (double aTargetX, double aTargetY, double aVelocityX, double aVelocityY, double &aReachedX, double &aReachedY);
    int	rangeFind (int &aAngle, int &aPower);

  public:
    double	type, type_saved, previous_type;
    double preftype;
    char* preftypeText[ALWAYS_PREF + 1];
    char *tank_type[8];
    char *teamText[3];

    PLAYER	*revenge;
    int	vengeful;	// 0-100 chance of retaliation
    double	vengeanceThreshold;	// Damage required to warrant revenge
    //   (% of max armour)
    double	defensive;	// -1.0 - 1.0, offense - defense
    double	annoyanceFactor;
    double	selfPreservation;	// Lengths gone to to avoid self-harm
    double	painSensitivity;	// How sensitive to damage
    int	rangeFindAttempts;
    int	retargetAttempts;
    double	focusRate;
    double	errorMultiplier;
    int	score;
    double	played, won;
    int     kills, killed;
    double	selected;
    int	money;
    double	damageMultiplier;
    TANK	*tank;
    int	color;
    void *pColor;
    int	color2;
    int 	nm[WEAPONS], ni[ITEMS];
    MENUENTRY	*menuopts;
    MENUDESC	*menudesc;
    char *typeText[LAST_PLAYER_TYPE];
    bool changed_weapon;
    bool gloating;
    double tank_bitmap;      // which type of tank do we have
    double team;
    int time_left_to_fire;
    bool skip_me;
    int last_shield_used;
    #ifdef NETWORK
    int server_socket;
    char net_command[NET_COMMAND_SIZE];
    #endif

    PLAYER (GLOBALDATA *global, ENVIRONMENT *env);
    // PLAYER (GLOBALDATA *global, ENVIRONMENT *env, ifstream &ifsFile, bool file); 
    ~PLAYER	();
    void	setName (char *name);
    char	*getName ()
    {
      return (_name);
    }
    int	calculateDirectAngle (int dx, int dy);
    void	exitShop ();
    void	newRound ();
    void	initialise ();
    TANK	*nextTank ();
    TANK	*currTank ();
    int	controlTank ();
    int	humanControls ();
    int	computerControls ();
    double	calcDefenseValue (TANK *ctank, TANK *ltank);
    double	selectTarget (); // select x to aim for
    double	selectTarget (int *targetXCoord, int *targetYCoord); // select x to aim for
    double  Select_Target (int *target_X, int *target_Y); // select x to aim for
    int	computerSelectItem (); // Choose weapon to fire
    int	chooseItemToBuy (int aMaxBoostValue = 0);
    void	generatePreferences ();
    void	setComputerValues (int aOffset = 0);
    int getMoneyToSave();
    int getBoostValue();
    int	selectRandomItem ();
    char	*selectRevengePhrase ();
    char	*selectGloatPhrase ();
    char    *selectSuicidePhrase ();
    int     saveToFile_Text (FILE *file);
    int     saveToFile (ofstream &ofsFile);
    int     loadFromFile_Text (FILE *file);
    int     loadFromFile (ifstream &ifsFile);
    void	initMenuDesc ();
    char    *Get_Team_Name();
    int     Select_Random_Weapon();
    int     Select_Random_Item();
    int    Reduce_Time_Clock();
    int    Buy_Something(int item_index);     // purchases the selected item
    int    Reclaim_Shield();    // restore unused shield

    #ifdef NETWORK
    void Trim_Newline(char *line);   // sanitize input
    int Get_Network_Command();
    int Execute_Network_Command(int my_turn);
    #endif
  };

#endif

