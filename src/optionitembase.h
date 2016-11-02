#pragma once
#ifndef ATANKS_SRC_OPTIONITEMBASE_H_INCLUDED
#define ATANKS_SRC_OPTIONITEMBASE_H_INCLUDED

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

#include "environment.h"
#include "globaldata.h"
#include "optiontypes.h"

#include <cassert>
#include <string>


/** @file optionitembase.h
  * @brief declaration of the option item base class
**/

extern uint32_t select_text_len; // needed for the item distribution

// Forward BUTTON if it isn't known, yet:
#ifndef BUTTON_HEADER_
class BUTTON;
#endif // BUTTON_HEADER_

// Forward Menu if it isn't known, yet:
#ifndef MENU_CLASS_DECLARES
class Menu;
#endif // MENU_CLASS_DECLARES


/** @class OptionItemBase
  * @brief Common base class for all option items.
  *
  * This base class holds all common values of the option item represented and
  * is used to order option items as a doubly linked list.
  *
  * The class has several public methods to change the inner state, like moving
  * or resizing the display area, changing the title, switching the text content
  * array and so on.
**/
class OptionItemBase
{
public:

	/* ------------------------------
	 * --- Public ctors and dtors ---
	 * ------------------------------
	 */

	explicit OptionItemBase(
				eEntryType    type_,
				const char*   title_,
				int32_t       titleIdx_,
				const char**  text_,
				int32_t       color_,
				eTextClass    class_,
				const char*   format_,
				int32_t top_, int32_t left_, int32_t width_, int32_t height_,
				int32_t padding_, int32_t show_size_);

	virtual ~OptionItemBase();


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void             clear_display(bool update_full);
	void             cursor_flip  ();
	void             getDimension (int32_t &tgt_width, int32_t &tgt_height);
	int32_t          getKeyCode   ();
	OptionItemBase*  getNext      ();
	OptionItemBase*  getPrev      ();
	uint32_t         getTextClass ();
	uint32_t         getTitleIdx  ();
	eEntryType       getType      ();
	void             insert_after (OptionItemBase* new_prev);
	void             insert_before(OptionItemBase* new_next);
	bool             is_click_in  (int32_t x, int32_t y, int32_t &ret);
	bool             is_selected  ();
	void             move         (int32_t new_left, int32_t new_top, bool do_update);
	bool             needs_text   ();
	void             remove       ();
	void             resize       (int32_t new_width, int32_t new_height);
	void             select       ();
	void             setPadding   (int32_t new_padding);
	void             setTitle     (const char*  new_title);
	void             setTextClass (eTextClass new_class);
	void             setTexts     (const char** new_texts);
	void             unselect     ();

	// virtuals to be implemented by the deriving template
	virtual int32_t activate    (int32_t val, int32_t x, int32_t y, int32_t k) =0;
	virtual bool    canGoDown   ()               =0;
	virtual bool    canGoUp     ()               =0;
	virtual void    display     (bool show_full) =0;
	virtual bool    isExitButton()               =0;


protected:

	/* -------------------------
	 * --- Protected methods ---
	 * -------------------------
	 */
	int32_t activateMenu(Menu* target);
	void    activateText(char* target, int32_t k);
	void    activateToggle(bool* target);
	void    displayButton();
	void    displayDeco(int32_t show_color = BLACK);
	void    displayMenu(Menu* target);
	void    displayText(char* target);
	void    displayText(const char* target);
	void    displayText(uint32_t* target);
	void    displayToggle(bool* target);

	/// @brief As OT_VALUE might be anything, it is templated on method scale.
	template<typename tgt_T>
	void displayValue(tgt_T* target)
	{
		if (format) {
			char txt_buf[256] = { 0x0 };
			snprintf(txt_buf, 255, format, *target);
			textLen = static_cast<int32_t>(strlen(txt_buf));
			this->displayText(txt_buf);
		} else if (texts && texts[entryNum]) {
			textLen = static_cast<int32_t>(strlen(texts[entryNum]));
			this->displayText(texts[entryNum]);
		}
	}

	// Note: The following templates only define a path for the dispatcher
	// when checking for which method to call. If a calling path, and thus any
	// option configuration, is invalid, they will print an error message and
	// terminate. These templates are only instantiated by the compiler for
	// syntax and call path checking, and thrown away being unused later.
	// This looks like a waste, but makes the dispatching a lot less complex
	// and more secure.
#define EMERGENCY_OUT fprintf(stderr, \
	"%s:%d [%s] : Illegal target type, template called!\n", \
	__FILE__, __LINE__, __FUNCTION__); \
	std::terminate();
	template<typename T> int32_t activateMenu(T*)      { EMERGENCY_OUT }
	template<typename T> void    activateText(T*, int) { EMERGENCY_OUT }
	template<typename T> void    activateToggle(T*)    { EMERGENCY_OUT }
	template<typename T> void    displayMenu(T*)       { EMERGENCY_OUT }
	template<typename T> void    displayText(T*)       { EMERGENCY_OUT }
	template<typename T> void    displayToggle(T*)     { EMERGENCY_OUT }
#undef EMERGENCY_OUT


	/* -------------------------
	 * --- Protected members ---
	 * -------------------------
	 */

	BUTTON*         button    = nullptr;    //!< The button used by ET_BUTTON.
	int32_t         color     = BLACK;      //!< The color to use for the text, mainly useful for OT_TOGGLE.
	bool            cursor_on = false;      //!< selected ET_TEXT elements feature a cursor.
	int32_t         curs_clk  = 0;          //!< Only react on every CURSOR_FLIP_TIME call.
	bool            decorated = false;      //!< Set to true by displayDeco() and false by clear_display(true)
	bool            drawn     = false;      //!< Set to true by display methods, and false by clear_display().
	int32_t         entryNum  = 0;          //!< Store the currently displayed text index with OT_VALUE options.
	const char*     format    = nullptr;    //!< Format string to use with OT_VALUE
	int32_t         height    = 0;          //!< Height of the display area.
	int32_t         keyCode   = 0;          //!< Key Code returned when clicking an ET_BUTTON.
	int32_t         left      = 0;          //!< Left x position of the display area.
	OptionItemBase* next      = nullptr;    //!< Next option entry in a doubly linked list.
	int32_t         padding   = 2;          //!< Padding of the title and possible buttons to the display area.
	OptionItemBase* prev      = nullptr;    //!< Previous option entry in a doubly linked list.
	bool            read_only = true;       //!< Whether ET_TEXT reacts on clicks and keys or not.
	bool            selected  = false;      //!< Whether this entry is selected or not.
	bool            show_menu = true;       //!< If set to true, the sub menu indicator is shown.
	int32_t         show_size = 0;          //!< Size of the color box ET_COLOR displays the current color in
	int32_t         textLen   = 0;          //!< Store the current size of OT_TEXT content.
	eTextClass      textClass = TC_NONE;    //!< Noted for language switch.
	bool            textOnly  = false;      //!< If set to true, displayText() draws no box.
	const char**    texts     = nullptr;    //!< Text array to use for OT_VALUE
	const char*     title     = nullptr;    //!< Title to display, mandatory
	int32_t         titleIdx  = 0;          //!< Noted for language switch. -1 means the title is fixed.
	int32_t         titleLen  = 0;          //!< Length of the title in pixels
	int32_t         top       = 0;          //!< Top y position of the display area.
	eEntryType      type      = ET_NONE;    //!< Type of the option, mandatory
	int32_t         width     = 0;          //!< Width of the display area.
};

#endif // ATANKS_SRC_OPTIONITEMBASE_H_INCLUDED

