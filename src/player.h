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
 *
 */


#include "player_types.h"
#include "globaltypes.h"

#define	MAX_WEAP_PROBABILITY 10000
#define BURIED_LEVEL         135
#define BURIED_LEVEL_HALF     68

#define NET_COMMAND_SIZE 64
// if we do not get a command after this amount of seconds,
// turn control over to the computer for a moment
#define NET_DELAY 1000
#define NET_DELAY_SHORT 500

class TANK;
class PLAYER;
class AICore;

/// @brief minimal struct to allow AI players to keep track of friend and foe.
struct sOpponent
{
	int32_t damage_from = 0;  //!< How much damage the opponent did to the player.
	int32_t damage_last = 0;  //!< How much damage the opponent did in this turn.
	int32_t damage_to   = 0;  //!< How much damage the player did to the opponent.
	double  fear        = 0.; //!< How likely evasive manoeuvres are started.
	double  fear_shock  = 0.; //!< The highest shock value determines the shocker.
	int32_t index       = -1; //!< Needed for saving/loading to work.
	int32_t killed_me   = 0;  //!< How many times this opponent has killed this player.
	int32_t killed_them = 0;  //!< How many times this opponent was killed by this player.
	PLAYER* opponent    = nullptr; //!< The PLAYER memorized here.
	int32_t revenge_dmg = 0;  //!< Summed up damage to determine when it is time for revenge.
};


/** @class PLAYER
  * @brief All data concerning human and A players
**/
class PLAYER
{
public:

	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	explicit PLAYER ();
	~PLAYER	();

	// no copying, no assignments
	PLAYER(const PLAYER&) =delete;
	PLAYER &operator=(const PLAYER&) =delete;


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void        checkOppMem         ();
	int32_t     chooseItemToBuy     (int32_t max_boost, int32_t &last_idx);
	eControl    controlTank         (AICore* aicore, bool allow_fire);
	void        drawIndicator       (int32_t x, int32_t y, int32_t h);
#ifdef NETWORK
	eControl    executeNetCmd       (bool my_turn, AICore* aicore);
#endif // NETWORK
	void	    exitShop            ();
	void        generatePreferences ();
	int32_t     getBoostValue       ();
	int32_t     getItemPref         (int32_t idx);
	int32_t     getMoneyToSave      (bool first_look);
	const char* getName             () const;
	bool        getNetCmd           ();
	sOpponent*  getOppMem           (int32_t idx);
	const char* getTeamName         () const;
	int32_t     getWeapPref         (int32_t idx);
	void	    initialise          (bool loaded_game);
	bool        load_from_file      (FILE* file);
	void        load_game_data      (FILE* file, int32_t file_version);
	void        newGame             ();
	void	    newRound            ();
	void        noteDamageFrom      (PLAYER* opponent, int32_t damage, bool destroyed);
	void        noteDamageTo        (PLAYER* opponent, int32_t damage, bool destroyed);
	void        reclaimShield       ();    // restore unused shield
	bool        reduceClock         ();
	void        save_game_data      (FILE* file);
	void        save_to_file        (FILE* file);
	const char* selectGloatPhrase   ();
	const char* selectPanicPhrase   (PLAYER* shocker);
	const char* selectKamikazePhrase();
	const char* selectRetaliationPhrase();
	const char* selectRevengePhrase ();
	const char* selectSuicidePhrase ();
	void        setLastOpponent     (sOpponent* last_opp);
	void	    setName             (const char* name_);
	void        updatePreferences   (int32_t max_boost, int32_t max_score);


	/* ----------------------
	 * --- Public members ---
	 * ----------------------
	 */

	int32_t        color              = BLACK;
	double         damageMultiplier   = 1.;
	double         defensive          = 0.; // [-1.0;1.0], offensive - defensive
	double         errorMultiplier    = 0.;
	bool           changed_weapon     = false;
	double         focusRate          = 0.;
	bool           gloating           = false;
	int32_t        index              = -1; // To note where in allPlayers this player is saved
	int32_t        killed             = 0;
	int32_t        kills              = 0;
	int32_t        last_shield_used   = 0;
	double         painSensitivity    = .5; // How sensitive to damage
	uint32_t       played             = 0;
	playerPrefType preftype           = PERPLAY_PREF;
	playerType     previous_type      = HUMAN_PLAYER;
	int32_t        money              = 15000;
	int32_t        ni[ITEMS];
	int32_t        nm[WEAPONS];
	PLAYER*        revenge            = nullptr;
	int32_t        score              = 0;
	abool_t        sdi_has_fired;           // Only one shot per frame
	int32_t        sdiShots           = 0;
	bool           selected           = false;
	double         selfPreservation   = .5; // Lengths gone to to avoid self-harm
	bool           skip_me            = false;
	TANK*          tank               = nullptr;
	int32_t        tankbitmap         = TT_NORMAL;
	eTeamTypes     team               = TEAM_NEUTRAL;
	int32_t        time_left_to_fire  = 0;
	playerType     type               = HUMAN_PLAYER;
	playerType     type_saved         = HUMAN_PLAYER;
	double         vengeanceThreshold = .5; // Damage required to warrant revenge
	int32_t        vengeful           = 50; // 0-100 chance of retaliation
	uint32_t       won                = 0;
#ifdef NETWORK
	int32_t        server_socket      = 0;
	char           net_command[NET_COMMAND_SIZE] = { 0 };
#endif // NETWORK


private:

	typedef ePlayerStages plStage_t;
	typedef sOpponent     opp_t;


	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	void        boostPrefences           (bool boostArmour, bool boostAmps,
	                                      bool boostWeapons);
	bool        buy_item                 (int32_t itemindex, int32_t max_boost);
	eControl    computerControls         (AICore *aicore, bool allow_fire);
	int32_t     computerSelectPreBuyItem (int32_t max_boost);
	int32_t     generateDesiredList      ();
	int32_t     getAmpValue              ();
	int32_t     getArmourValue           ();
	eControl    humanControls            (AICore* aicore);


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	int32_t   boostBought      = -1;
	int32_t   currPref[THINGS];     // current preferences, calculated for each round
	int32_t   desired[THINGS];      // Shopping wish list
	int32_t   saveMoneyFor[THINGS]; // List of items the AI wants to buy
	opp_t*    last_opponent    = nullptr;
	char      name[NAME_LEN + 1];
	bool      needAmp          = false;
	bool      needArmour       = false;
	bool      needDamage       = false;
	int32_t   oppCount         = 0;
	opp_t*    opponents        = nullptr;
	plStage_t plStage          = PS_SELECT_WEAPON;
	int32_t   shieldBought     = -1;
	int32_t   weapPref[THINGS]; // Static preferences, generated once
};


// For headers including player.h to know that the class is there:
#define HAS_PLAYER 1
// Note: Due to circular dependencies, some headers might need to forward
//       the player class.


/** @struct PLAYER_mini
  * @brief Minimum dataset of editable values.
  *
  * This minimal struct is used for the player editing menu.
  *   - When adding a new player it allows to cancel the addition without
  * atanks to first call new and then delete. Further it keeps the last
  * settings so adding many new players with the same settings but different
  * names becomes very easy.
 **/
struct PLAYER_mini
{
	int32_t        color          = GREEN;
	int32_t        index          = -1;
	char	       name[NAME_LEN];
	uint32_t       played         = 0;
	PLAYER*        player         = nullptr;
	playerPrefType preftype       = ALWAYS_PREF;
	int32_t        tankbitmap     = TT_NORMAL;
	eTeamTypes     team           = TEAM_NEUTRAL;
	playerType     type           = HUMAN_PLAYER;
	uint32_t       won            = 0;

	// a ctor, needed by VisualC++ for the name.
	explicit PLAYER_mini();

	// "Backup a player"
	void copy_from(PLAYER* source);

	// Write back the values
	void write_back(PLAYER* target = nullptr);
};

#define HAS_PLAYER_MINI 1

// Helper functions to be used as action function with ET_BUTTON entries
int32_t edit_player(PLAYER** target, int32_t);
int32_t new_player (PLAYER** target, int32_t);


#endif

