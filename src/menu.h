#pragma once
#ifndef ATANKS_SRC_MENU_H_INCLUDED
#define ATANKS_SRC_MENU_H_INCLUDED

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

#include "optionitem.h"
#include "optionitemmenu.h"
#include "optionitemplayer.h"
#include "button.h"
#include <new> // for bad_alloc exception


/** @file menu.h
  * @brief Declare Menu class for self managing menus
**/


/** @enum eMenuReturnCodes
  * @brief Standard return codes for the main loop
**/
enum eMenuReturnCodes {
	MRC_None = 0,
	MRC_Play_Game,
	MRC_Load_Game,
	MRC_Esc_Menu
};


/** @class Menu
  * @brief A class to build menus out of option items.
  *
  * @todo : Write more
**/
class Menu
{
public:

	/* -------------------------------------------
	 * --- Public constructors and destructors ---
	 * -------------------------------------------
	 */

	explicit Menu(eMenuClass class_, int32_t menuX, int32_t menuY);
	~Menu();



	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	// Add a button without action function.
	int32_t addButton(int32_t title_idx, const char* title_, int32_t key_code,
	                  BITMAP* bmp, BITMAP* hover,
	                  BITMAP* released, bool text_only,
	                  int32_t left, int32_t top, int32_t width, int32_t height,
	                  int32_t padding);


	// Add a color option
	int32_t addColor(int32_t* target, int32_t title_idx,
	                 int32_t left, int32_t top, int32_t width, int32_t height,
	                 int32_t show_size, int32_t padding);


	// Add a sub menu option with Menu target
	int32_t addMenu(Menu* menu, int32_t title_idx, int32_t color,
	                int32_t left, int32_t top, int32_t width, int32_t height,
	                int32_t padding);


	// Add a sub menu option with PLAYER target (set title_idx to -1 to use player name)
	int32_t addMenu(PLAYER** player,
	                int32_t (*action_)(PLAYER** player_, int32_t),
	                int32_t title_idx,
	                int32_t left, int32_t top, int32_t width, int32_t height,
	                int32_t padding);


	// Special minimum variant for editable text options
	int32_t addText(char* target, int32_t title_idx, uint32_t max_len,
	                int32_t color, const char* format,
	                int32_t left, int32_t top, int32_t width, int32_t height,
	                int32_t padding);


	/** @brief This adds a text option with readonly text
	  *
	  * Please note: The position @a left / @a top are relative to
	  * the menu position.
	  *
	  * Please also note that a title is displayed to the left of the display
	  * area.
	  *
	  * @param[in] target Pointer to the target to display.
	  * @param[in] title_idx Index of the title if it is listed in MenuTitleText.
	  * @param[in] color Regular display color of the title/target.
	  * @param[in] format The format to represent the target, used by snprintf().
	  * @param[in] left Relative left position of the display area to the menu.
	  * @param[in] top Relative top position of the display area to the menu.
	  * @param[in] width Width of the display area. The real width might be larger.
	  * @param[in] height Height of the display area.
	  * @param[in] padding Distance between title and display.
	**/
	template<typename tgt_T>
	int32_t addText(tgt_T* target, int32_t title_idx,
	                int32_t color, const char* format,
	                int32_t left, int32_t top, int32_t width, int32_t height,
	                int32_t padding)
	{
		OptionItemBase* curr = nullptr;
		bool title_valid = is_title_idx_valid(title_idx);

		assert (title_valid && "ERROR: The given title index is invalid");

		if (target && title_valid) {
			try {
				curr = new OptionItem<tgt_T, int32_t>(
									target, 0, color, ET_TEXT,
									"", title_idx, format,
									menu_y + top, menu_x + left, width, height,
									padding);
			} catch (std::bad_alloc &e) {
				cerr << __FUNCTION__ << " : failed to allocate new TEXT OptionItem\n";
				cerr << " [" << e.what() << "]" << endl;
			}
		}

		return this->insert_option(curr, title_idx, nullptr);
	}


	// Special minimum variant for toggle types feeding a bool with variable title
	int32_t addToggle(bool* target, int32_t title_idx, int32_t color,
	                  int32_t left, int32_t top, int32_t width, int32_t height,
	                  int32_t padding);


	// Special minimum variant for toggle types feeding a bool with fixed title
	int32_t addToggle(bool* target, const char* title_, int32_t color,
	                  int32_t left, int32_t top, int32_t width, int32_t height,
	                  int32_t padding);


	// Special minimum variant for toggle types handling PLAYER::selected
	int32_t addToggle(PLAYER** player,
	                  int32_t left, int32_t top, int32_t width, int32_t height,
	                  int32_t padding);


	/** @brief Simple ET_VALUE option with direct value representation
	  *
	  * Please note: The position @a left / @a top are relative to
	  * the menu position.
	  *
	  * Please also note that a title is displayed to the left of the display
	  * area and wheel buttons to the right.
	  *
	  * @param[in] target Pointer to the target to handle.
	  * @param[in] title_idx Index of the title if it is listed in MenuTitleText.
	  * @param[in] color Regular display color of the title/target.
	  * @param[in] minimum Minimum value for ET_VALUE targets.
	  * @param[in] maximum Maximum value for ET_VALUE targets.
	  * @param[in] increment Value to increment/decrement the target on activation.
	  * @param[in] format printf format that can pretty print @a target.
	  * @param[in] left Relative left position of the display area to the menu.
	  * @param[in] top Relative top position of the display area to the menu.
	  * @param[in] width Width of the display area. The real width might be larger.
	  * @param[in] height Height of the display area.
	  * @param[in] padding Distance between title, display and wheel buttons.
	**/
	template<typename tgt_T, typename opt_T = int32_t>
	int32_t addValue(tgt_T* target, int32_t title_idx, int32_t color,
	                 opt_T minimum, opt_T maximum, opt_T increment,
	                 const char* format,
	                 int32_t left, int32_t top, int32_t width, int32_t height,
	                 int32_t padding)
	{
		OptionItemBase* curr = nullptr;

		if (target) {
			try {
				curr = new OptionItem<tgt_T, opt_T>(
								target, "", title_idx,
								nullptr, color, TC_NONE,
								minimum, maximum, increment, format,
								menu_y + top, menu_x + left, width, height,
								padding, nullptr);
			} catch (std::bad_alloc &e) {
				cerr << __FUNCTION__ << " : failed to allocate new OptionItem\n";
				cerr << " [" << e.what() << "]" << endl;
			}
		}

		return this->insert_option(curr, title_idx, nullptr);
	}


	/** @brief Simple option with text array representation
	  *
	  * Please note: The position @a left / @a top are relative to
	  * the menu position.
	  *
	  * Please also note that a title is displayed to the left of the display
	  * area.
	  *
	  * @param[in] target Pointer to the target to handle.
	  * @param[in] title_idx Index of the title if it is listed in MenuTitleText.
	  * @param[in] texts Free text array.
	  * @param[in] color Regular display color of the title/target.
	  * @param[in] text_class The text class, set to TC_FREETEXT to use @a texts.
	  * @param[in] maximum Maximum value for ET_VALUE targets.
	  * @param[in] left Relative left position of the display area to the menu.
	  * @param[in] top Relative top position of the display area to the menu.
	  * @param[in] width Width of the display area. The real width might be larger.
	  * @param[in] height Height of the display area.
	  * @param[in] padding Distance between title and display.
	**/
	template<typename tgt_T, typename opt_T = int32_t>
	int32_t addValue(tgt_T* target, int32_t title_idx,
	                 const char** texts, int32_t color,
	                 eTextClass text_class, opt_T maximum,
	                 int32_t left, int32_t top, int32_t width, int32_t height,
	                 int32_t padding)
	{
		OptionItemBase* curr = nullptr;

		if (target) {
			try {
				curr = new OptionItem<tgt_T, opt_T>(
								target, "", title_idx,
								nullptr, color, TC_NONE, 0, maximum, 1, nullptr,
								menu_y + top, menu_x + left, width, height,
								padding, nullptr);
				this->setTexts(curr, texts, text_class);
			} catch (std::bad_alloc &e) {
				cerr << __FUNCTION__ << " : failed to allocate new OptionItem\n";
				cerr << " [" << e.what() << "]" << endl;
			}
		}

		return this->insert_option(curr, title_idx, nullptr);
	}


	/** @brief Value option with text array representation and display function
	  *
	  * Please note: The position @a left / @a top are relative to
	  * the menu position.
	  *
	  * Please also note that a title is displayed to the left of the display
	  * area.
	  *
	  * @param[in] target Pointer to the target to handle.
	  * @param[in] title_idx Index of the title if it is listed in MenuTitleText.
	  * @param[in] texts Free text array.
	  * @param[in] color Regular display color of the title/target.
	  * @param[in] text_class The text class, set to TC_FREETEXT to use @a texts.
	  * @param[in] maximum Maximum value for ET_VALUE targets.
	  * @param[in] left Relative left position of the display area to the menu.
	  * @param[in] top Relative top position of the display area to the menu.
	  * @param[in] width Width of the display area. The real width might be larger.
	  * @param[in] height Height of the display area.
	  * @param[in] padding Distance between title and display.
	  * @param[in] display_ optional display function to use.
	**/
	template<typename tgt_T, typename opt_T = int32_t>
	int32_t addValue(tgt_T* target, int32_t title_idx,
	                 const char** texts, int32_t color,
	                 eTextClass text_class, opt_T maximum,
	                 int32_t left, int32_t top, int32_t width, int32_t height,
	                 int32_t padding,
	                 bool (*display_)(tgt_T* target, int32_t x, int32_t y) )
	{
		OptionItemBase* curr = nullptr;

		if (target) {
			try {
				curr = new OptionItem<tgt_T, opt_T>(
								target, "", title_idx,
								nullptr, color, TC_NONE, 0, maximum, 1, nullptr,
								menu_y + top, menu_x + left, width, height,
								padding, display_);
				this->setTexts(curr, texts, text_class);
			} catch (std::bad_alloc &e) {
				cerr << __FUNCTION__ << " : failed to allocate new OptionItem\n";
				cerr << " [" << e.what() << "]" << endl;
			}
		}

		return this->insert_option(curr, title_idx, nullptr);
	}


	/** @brief Value option with text array representation and action function
	  *
	  * Please note: The position @a left / @a top are relative to
	  * the menu position.
	  *
	  * Please also note that a title is displayed to the left of the display
	  * area.
	  *
	  * @param[in] target Pointer to the target to handle.
	  * @param[in,out] action_ Pointer to the action function handling the wheel button click.
	  * @param[in] title_idx Index of the title if it is listed in MenuTitleText.
	  * @param[in] texts Free text array.
	  * @param[in] color Regular display color of the title/target.
	  * @param[in] text_class The text class, set to TC_FREETEXT to use @a texts.
	  * @param[in] maximum Maximum value for ET_VALUE targets.
	  * @param[in] left Relative left position of the display area to the menu.
	  * @param[in] top Relative top position of the display area to the menu.
	  * @param[in] width Width of the display area. The real width might be larger.
	  * @param[in] height Height of the display area.
	  * @param[in] padding Distance between title and display.
	**/
	template<typename tgt_T, typename opt_T = int32_t>
	int32_t addValue(tgt_T* target,
	                 int32_t (*action_)(tgt_T* target, int32_t val),
	                 int32_t title_idx,
	                 const char** texts, int32_t color,
	                 eTextClass text_class, opt_T maximum,
	                 int32_t left, int32_t top, int32_t width, int32_t height,
	                 int32_t padding)
	{
		OptionItemBase* curr = nullptr;

		if (target) {
			try {
				curr = new OptionItem<tgt_T, opt_T>(
								target, action_, ET_VALUE,
								"", title_idx, nullptr, color, TC_NONE,
								0, maximum, 1, nullptr,
								menu_y + top, menu_x + left, width, height,
								padding, nullptr);
				this->setTexts(curr, texts, text_class);
			} catch (std::bad_alloc &e) {
				cerr << __FUNCTION__ << " : failed to allocate new OptionItem\n";
				cerr << " [" << e.what() << "]" << endl;
			}
		}

		return this->insert_option(curr, title_idx, nullptr);
	}


	void            clearAll    (bool full_clear);
	int32_t         count       ();
	int32_t         delete_entry(int32_t index);
	void            displayAll  (bool full_display);
	void            distribute  (int32_t first_idx, int32_t last_idx,
	                             int32_t list_width, int32_t list_height,
	                             int32_t y_off, bool do_update);
	OptionItemBase* getSelected ();
	const char*     getTitle    () const;
	void            move_entry  (int32_t from_idx, int32_t to_idx);
	void            redraw      (int32_t index, bool update_full);
	void            redrawAll   (bool full_redraw);
	void            setLanguage (bool autorefresh);
	void            setTitle    (const char* new_title, bool autorefresh);


	/* ------------------------
	 * --- Public operators ---
	 * ------------------------
	 */

	// operator() to use a menu instance like a function
	int32_t operator()();

	// Get a stored option by index
	OptionItemBase* operator[](int32_t index);

private:

	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	int32_t insert_option     (OptionItemBase* new_opt);
	int32_t insert_option     (OptionItemBase* new_opt, int32_t title_idx,
	                           const char* title_);
	bool    is_title_idx_valid(int32_t title_idx);
	int32_t selectClicked     (int32_t x, int32_t y);
	void    selectNext        ();
	void    selectPrev        ();
	void    setTexts          (OptionItemBase* item, const char** texts,
	                           eTextClass text_class);
	void    unselect          ();

	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	int32_t          bgItems    = 0;
	int32_t          bgOffset   = 0;
	eBackgroundTypes bgType     = BACKGROUND_BLANK;
	int32_t          entry_cnt  = 0;          //!< Number of entries currently in the list.
	int32_t          entry_sel  = -1;         //!< Currently selected entry or -1 if none is selected.
	eMenuClass       menu_class = MC_MAIN;    //!< The class of the menu, decides upon what to display.
	eLanguages       menu_lang  = EL_ENGLISH; //!< The language to display
	int32_t          menu_x     = 0;          //!< X-Pos where the menu background starts
	int32_t          menu_y     = 0;          //!< Y-Pos where the menu background starts
	OptionItemBase*  root       = nullptr;    //!< The first menu item
	OptionItemBase*  tail       = nullptr;    //!< The last menu item
	const char*      title      = nullptr;    //!< Name/Title of the menu
	uint32_t         title_len  = 0;          //!< Length of the menu title with the current font.
	bool             title_set  = false;      //!< Set to true if this has been changed to be an individual title
	int32_t          title_x    = 0;
};
#define MENU_CLASS_DECLARES 1


// --- Helper functions for action/display usage that need optioncontent.h ---

/// @brief display function to display the chosen tank at a specific location
bool display_tank_desc(int32_t* tanknum, int32_t x, int32_t y);



#endif // ATANKS_SRC_MENU_H_INCLUDED

