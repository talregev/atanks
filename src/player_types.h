#pragma once
#ifndef ATANKS_SRC_PLAYER_TYPES_H_INCLUDED
#define ATANKS_SRC_PLAYER_TYPES_H_INCLUDED

/** @file player_types.h
  * @brief used enums plus operators for players and tanks
**/

#include "main.h"

enum ePlayerStages
{
	PS_AI_IS_IDLE = 0, //!< AI has nothing to do and is free to get work
	PS_AI_INITIALIZE,  //!< AI is initializing its data
	PS_SELECT_TARGET,  //!< AI is selecting a target
	PS_SELECT_WEAPON,  //!< AI is selecting a weapon or item
	PS_CALCULATE,      //!< AI calculates its basic attack values
	PS_AIM,            //!< AI aims the current selection to hit its target
	PS_MOVE_LEFT,      //!< AI wants to move their tank to the left
	PS_MOVE_RIGHT,     //!< AI wants to move their tank to the right
	PS_FIRE,           //!< AI is ready to have the current weapon/item fired
	PS_CLEANUP,        //!< AI is cleaning up
	PS_STAGE_COUNT
};

ePlayerStages &operator+=(ePlayerStages &src, int32_t val);
ePlayerStages &operator-=(ePlayerStages &src, int32_t val);
ePlayerStages &operator++(ePlayerStages &src);          // pre
ePlayerStages  operator++(ePlayerStages &src, int32_t); // post

enum playerType
{
	HUMAN_PLAYER = 0,
	USELESS_PLAYER,
	GUESSER_PLAYER,
	RANGEFINDER_PLAYER,
	TARGETTER_PLAYER,
	DEADLY_PLAYER,
	LAST_PLAYER_TYPE,
	PART_TIME_BOT,         // normally a human, but acting as a deadly computer
	VERY_PART_TIME_BOT,    // just fires one shot
	NETWORK_CLIENT,
	SDI_PREDICTOR          // Used so missile mind shots from the SDI won't
	                       // trigger another SDI check, trigger another SDI
	                       // check, trigger another...
};

playerType &operator+=(playerType &src, int32_t val);
playerType &operator-=(playerType &src, int32_t val);
playerType &operator++(playerType &src);          // pre
playerType  operator++(playerType &src, int32_t); // post

// player weapon preference type
// ALWAYS_PREF - only choose weapon preferences once on player creation
// PERPLAY_PREF - choose weapon preferences once per game
enum playerPrefType
{
	PERPLAY_PREF = 0,
	ALWAYS_PREF,
	PREF_COUNT
};

playerPrefType &operator+=(playerPrefType &src, int32_t val);
playerPrefType &operator-=(playerPrefType &src, int32_t val);
playerPrefType &operator++(playerPrefType &src);          // pre
playerPrefType  operator++(playerPrefType &src, int32_t); // post


/** @enum ePlayerEdit
  * @brief return codes used by the sub menus when editing players
**/
enum ePlayerEdit
{
	PE_BACK         = 1,        //!< User opted out. No new player, no edit and no deletion.
	PE_CONFIRM_NEW  = 0x010000, //!< Adding a new player was confirmed
	PE_CONFIRM_EDIT = 0x020000, //!< Changes to a player have been confirmed
	PE_CONFIRM_DEL  = 0x040000  //!< Deleting a player was confirmed
	// Note: The values allow to use the last 16 bit for key code bit masks.
};

/** @enum eTeamTypes
  * @brief determines the team a player belongs to
**/
enum eTeamTypes
{
	TEAM_SITH    = 0,
	TEAM_NEUTRAL,
	TEAM_JEDI,
	TEAM_COUNT
};

eTeamTypes &operator+=(eTeamTypes &src, int32_t val);
eTeamTypes &operator-=(eTeamTypes &src, int32_t val);
eTeamTypes &operator++(eTeamTypes &src);          // pre
eTeamTypes  operator++(eTeamTypes &src, int32_t); // post

/** @enum eTankOffsets
  * @brief Centrally store the bitmap offsets of the tank images
**/
enum eTankOffsets
{
	TO_TURRET = 0,
	TO_TANK   = 7
};


/** @enum eTankTypes
  * @brief the tanks currently known, TT_TANK_COUNT is the number of tanks
**/
enum eTankTypes
{
	TT_NORMAL = 0,
	TT_CLASSIC,
	TT_BIGGREY,
	TT_T34,
	TT_HEAVY,
	TT_FUTURE,
	TT_UFO,
	TT_SPIDER,
	TT_BIGFOOT,
	TT_MINI,
	TT_TANK_COUNT
};

eTankTypes &operator+=(eTankTypes &src, int32_t val);
eTankTypes &operator-=(eTankTypes &src, int32_t val);
eTankTypes &operator++(eTankTypes &src);          // pre
eTankTypes  operator++(eTankTypes &src, int32_t); // post


#endif // ATANKS_SRC_PLAYER_TYPES_H_INCLUDED

