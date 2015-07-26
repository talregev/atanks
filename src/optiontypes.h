#pragma once
#ifndef ATANKS_SRC_OPTIONTYPES_H_INCLUDED
#define ATANKS_SRC_OPTIONTYPES_H_INCLUDED

/*
 * atanks - obliterate each other with oversize weapons
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your menu) any later version.
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

/** @file optiontypes.h
  *
  * This file defines enums used for the menus.
**/


#include <string>


/** @enum eMenuClass
  * @brief List of menu classes. Every menu class is a menu in itself.
  *
  * MC_MENUCLASS_COUNT can be used to retrieve the number of menu classes.
  *
  * This enum is sorted in alphabetical order to make maintenance easier.
**/
enum eMenuClass
{
	MC_AREYOUSURE = 0, //!< The "Are you sure?" <yes> <no> screen
	MC_FINANCE,        //!< The finance ("Money") options sub menu
	MC_GRAPHICS,       //!< The graphics options sub menu
	MC_MAIN,           //!< Menu shown when using "Options" button
	MC_NETWORK,        //!< The network options sub menu
	MC_PHYSICS,        //!< The physics options sub menu
	MC_PLAY,           //!< Menu shown when using "Play" button
	MC_PLAYER,         //!< The player edit menu
	MC_PLAYERS,        //!< Menu shown when hitting "Players" button
	MC_RESET,          //!< The "Reset options?" <Reset> <Back> screen
	MC_SOUND,          //!< The sound options sub menu
	MC_WEATHER,        //!< The weather options sub menu
	MC_MENUCLASS_COUNT
};


/** @enum eTextClass
  * @brief Declare the menu option text classes.
  *
  * These are used so repeating texts do not need to be translated over and
  * over again.
  * TC_TEXTCLASS_COUNT can be used to retrieve the number of
  * fixed menu entry text classes in OptionClassText[][][].
  *
  * This enum is sorted alphabetically to make maintenance easier.
**/
enum eTextClass
{
	TC_COLOUR = 0,
	TC_LANDSLIDE,
	TC_LANDTYPE,
	TC_LANGUAGE,
	TC_LIGHTNING,
	TC_METEOR,
	TC_MOUSE,
	TC_OFFON,
	TC_OFFONRANDOM,
	TC_PLAYERPREF,
	TC_PLAYERTEAM,
	TC_PLAYERTYPE,
	TC_SATELLITE,
	TC_SKIPTYPE,
	TC_SOUNDDRIVER,
	TC_TANKTYPE,
	TC_TURNTYPE,
	TC_WALLTYPE,
	TC_TEXTCLASS_COUNT, //!< First value after fixed text class list
	TC_FREETEXT,        //!< Special value for dynamically composed text arrays
	TC_NONE             //!< Special value for no text class at all
};


/** @enum eEntryType
  * @brief Declare the different entry types option items can have
**/
enum eEntryType
{
	ET_NONE = 0,
	ET_ACTION,
	ET_BUTTON,
	ET_COLOR,
	ET_MENU,
	ET_OPTION,
	ET_TEXT,
	ET_TOGGLE,
	ET_VALUE
};


/** @enum eResetOptions
  * @brief return codes for the "Are you sure" reset button question
**/
enum eResetOptions
{
	RO_BACK  = 667,
	RO_RESET = 1337
};


// Some helper functions to get names for enum entries
std::string getEntryTypeName(eEntryType etype);
std::string getMenuClassName(eMenuClass mclass);
std::string getTextClassName(eTextClass tclass);


#endif // ATANKS_SRC_OPTIONTYPES_H_INCLUDED

