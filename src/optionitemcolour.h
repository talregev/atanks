#pragma once
#ifndef ATANKS_SRC_OPTIONITEMCOLOUR_H_INCLUDED
#define ATANKS_SRC_OPTIONITEMCOLOUR_H_INCLUDED

/*
 * atanks - obliterate each other with oversize weapons
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

#include "optionitembase.h"

/** @file optionitemcolour.h
  * @brief declaration of the option entry class specialized on handling
  * int32_t instances representing colours.
**/


/** @class OptionItemColour
  * @brief abstract one option menu entry to handle an int32_t instance
  *
  * This class is a special version of the OptionItem template that can only
  * handle int32_t instances representing colours.
  *
  * The the only entry type supported is the ET_COLOR.
**/
class OptionItemColour : public OptionItemBase
{
public:

	/* -------------------------------------------
	 * --- Public constructors and destructors ---
	 * -------------------------------------------
	 */

	explicit
	OptionItemColour(
				int32_t*      color_,
				const char*   title_,
				int32_t       titleIdx_,
				int32_t top_, int32_t left_, int32_t width_, int32_t height_,
				int32_t padding_, int32_t show_size_);
	virtual ~OptionItemColour();

	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	virtual int32_t activate    (int32_t, int32_t, int32_t, int32_t);
	virtual bool    canGoDown   ();
	virtual bool    canGoUp     ();
	virtual void    display     (bool show_full);
	virtual bool    isExitButton();
	void            setLanguage ();

private:

	/* ----------------------------------------------
	 * --- Private methods and external functions ---
	 * ----------------------------------------------
	 */


	void displayCross();


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	int32_t  act_x      = 0;
	int32_t  act_y      = 0;
	BITMAP*  tgt_bitmap = nullptr; //!< Pre-drawn Rainbow box
	int32_t* tgt_color  = nullptr; //!< Colour instance to handle
};


#endif // ATANKS_SRC_OPTIONITEMCOLOUR_H_INCLUDED

