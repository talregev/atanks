#include "optioncontent.h"
#include "optionitemcolour.h"
#include "main.h"
#include "button.h"
#include "menu.h"
#include "player.h"
#include "externs.h"
#include "clock.h"

#include <cassert>
#include <exception>


void flush_inputs(); // From files.h
void init_mouse_cursor(); // from atanks.cpp to change the mouse cursor

static int32_t MOUSE_RELEASE_DELAY = 20; // Slows down constant mouse presses
static int32_t MOUSE_DELAY_REDUCT  = 5;  // Every so many rounds the delay is reduced


/* -------------------------------------------
 * --- Public constructors and destructors ---
 * -------------------------------------------
 */

Menu::Menu(eMenuClass class_, int32_t menuX, int32_t menuY) :
	menu_class(class_),
	menu_x(menuX),
	menu_y(menuY)
{
	// Save here to detect language changes.
	menu_lang = env.language;

	assert ( (menu_class < MC_MENUCLASS_COUNT)
	      && "ERROR: class_ must be smaller than MC_MENUCLASS_COUNT");

	// Set title according to class and language:
	title     = MenuTitleText[menu_class][menu_lang][0];
	title_len = text_length(font, title);
	title_x   = menu_x + text_length(font, "W") + 2;

	// Set background style
	bgType   = env.dynamicMenuBg
	         ? static_cast<eBackgroundTypes>(rand() % BACKGROUND_COUNT)
	         : BACKGROUND_BLANK;
	bgOffset = (RAND_MAX / 4) + (rand() % (RAND_MAX / 4));
	bgItems  = (rand() % 100) + 20;
}


Menu::~Menu()
{
	// Remove all options
	while (entry_cnt > 0) {
		OptionItemBase* curr = tail;
		tail = curr->getPrev();
		delete curr; // Removes it automatically
		--entry_cnt;
	}

	// Delete title if it was set manually
	if (title_set && title)
		free (const_cast<char*>(title));
}



/* ----------------------
 * --- Public methods ---
 * ----------------------
 */


/** @brief add a button to the menu
  *
  * This button shows a bitmap with text on it and returns a key code when
  * clicked on.
  *
  * If no bitmaps are defined, a light button is drawn "by hand".
  *
  * Please note: The position @a left / @a top are relative to
  * the menu position.
  *
  * @param[in] title_idx index of the title text if it is listed in MenuTitleText.
  * @param[in] title_ Pointer to a fixed title to be used instead of an indexed one.
  * @param[in] key_code The key code to return if the button is clicked.
  * @param[in] bmp Bitmap to use for regular display.
  * @param[in] hover Bitmap to use when the mouse pointer hovers over the button.
  * @param[in] released Bitmap to use when the button was clicked on.
  * @param[in] text_only If set to true the button is only its title text.
  * @param[in] left Relative left position of the display area to the menu.
  * @param[in] top Relative top position of the display area to the menu.
  * @param[in] width Width of the display area. The real width might be larger.
  * @param[in] height Height of the display area.
  * @param[in] padding Distance between title, display and wheel buttons.
  * @return Number of options in the menu after adding the button.
**/
int32_t Menu::addButton(int32_t title_idx, const char* title_, int key_code,
                        BITMAP* bmp, BITMAP* hover, BITMAP* released,
                        bool text_only, int left, int top, int width, int height,
                        int padding)
{
	OptionItemBase* curr        = nullptr;
	BUTTON*         btn         = nullptr;
	bool            title_valid = is_title_idx_valid(title_idx);

	// 1) Create the button
	try {
		if (bmp || hover || released)
			btn = new BUTTON(title_ ? title_
			                 : title_valid
			                   ? MenuTitleText[menu_class][menu_lang][title_idx]
			                   : nullptr,
			                 text_only, menu_x + left, menu_y + top,
			                 bmp, hover, released);
		else
			btn = new BUTTON(title_ ? title_
			                 : title_valid
			                   ? MenuTitleText[menu_class][menu_lang][title_idx]
			                   : nullptr,
			                 text_only, menu_x + left, menu_y + top, width, height);
	} catch (std::bad_alloc &e) {
		cerr << __FUNCTION__ << " : failed to allocate new BUTTON\n";
		cerr << " [" << e.what() << "]" << endl;
	}

	// 2) Create the option
	try {
		curr = new OptionItem<int, int>(key_code, nullptr,
		                                nullptr,
		                                title_ ? title_
		                                : title_valid
		                                  ? MenuTitleText[menu_class][menu_lang][title_idx]
		                                  : "",
		                                title_idx, btn,
		                                left, top, width, height, padding);
	} catch (std::bad_alloc &e) {
		cerr << __FUNCTION__ << " : failed to allocate new ET_BUTTON option\n";
		cerr << " [" << e.what() << "]" << endl;
	}

	// 3) Insert option:
	return this->insert_option(curr);
}


/** @brief This adds a color option to pick a color value
  *
  * Please note: The position @a left / @a top are relative to
  * the menu position.
  *
  * @param[in] target Pointer to the target to handle.
  * @param[in] title_idx Index of the title if it is listed in MenuTitleText.
  * @param[in] left Relative left position of the display area to the menu.
  * @param[in] top Relative top position of the display area to the menu.
  * @param[in] width Width of the display area. The real width might be larger.
  * @param[in] height Height of the display area.
  * @param[in] show_size Border length of the square displaying the currently picked color.
  * @param[in] padding Distance between title and display.
**/
int32_t Menu::addColor(int32_t* target, int32_t title_idx,
                       int32_t left, int32_t top, int32_t width, int32_t height,
                       int32_t show_size, int32_t padding)
{
	OptionItemBase* curr = nullptr;
	bool title_valid = is_title_idx_valid(title_idx);

	assert (title_valid && "ERROR: The given title index is invalid");

	if (target && title_valid) {
		try {
			curr = new OptionItemColour(
			                    target,
			                    MenuTitleText[menu_class][menu_lang][title_idx],
			                    title_idx,
			                    menu_y + top, menu_x + left, width, height,
			                    padding, show_size);
		} catch (std::bad_alloc &e) {
			cerr << __FUNCTION__ << " : failed to allocate new ET_COLOR OptionItem\n";
			cerr << " [" << e.what() << "]" << endl;
		}
	}

	return this->insert_option(curr);
}


/** @brief This adds a sub menu option with direct menu access
  *
  * Please note: The position @a left / @a top are relative to
  * the menu position.
  *
  * @param[in] menu Pointer to the menu to handle.
  * @param[in] title_idx Index of the title if it is listed in MenuTitleText.
  * @param[in] color Regular display color of the title.
  * @param[in] left Relative left position of the display area to the menu.
  * @param[in] top Relative top position of the display area to the menu.
  * @param[in] width Width of the display area. The real width might be larger.
  * @param[in] height Height of the display area.
  * @param[in] padding Distance between title and display.
  * @return Number of options in the menu after adding the button.
**/
int32_t Menu::addMenu(Menu* menu, int32_t title_idx, int32_t color,
                      int32_t left, int32_t top, int32_t width, int32_t height,
                      int32_t padding)
{
	OptionItemBase* curr = nullptr;
	bool title_valid = is_title_idx_valid(title_idx);

	assert (title_valid && "ERROR: The given title index is invalid");

	if (menu && title_valid) {
		try {
			curr = new OptionItemMenu(menu,
			                    MenuTitleText[menu_class][menu_lang][title_idx],
			                    title_idx, color,
			                    menu_y + top, menu_x + left, width, height,
			                    padding);
		} catch (std::bad_alloc &e) {
			cerr << __FUNCTION__ << " : failed to allocate new ET_MENU OptionItem\n";
			cerr << " [" << e.what() << "]" << endl;
		}
	}

	return this->insert_option(curr);
}


/** @brief This adds a sub menu option that handles a player (edit/create)
  *
  * Important: This _MUST_ have an action function that does the real work. This
  * method ads an OptionItemPlayer instance, which is only a bridge to the
  * action function.
  *
  * Please note: The position @a left / @a top are relative to
  * the menu position.
  *
  * @param[in,out] player_ Pointer to the PLAYER instance to handle.
  * @param[in,out] action_ Pointer to the action function handling the button click.
  * @param[in] title_idx Index of the title if it is listed in MenuTitleText.
  * @param[in] left Relative left position of the display area to the menu.
  * @param[in] top Relative top position of the display area to the menu.
  * @param[in] width Width of the display area. The real width might be larger.
  * @param[in] height Height of the display area.
  * @param[in] padding Distance between title and display.
  * @return Number of options in the menu after adding the button.
**/
int32_t Menu::addMenu(PLAYER** player,
                      int32_t (*action_)(PLAYER** player_, int32_t),
                      int32_t title_idx,
                      int32_t left, int32_t top, int32_t width, int32_t height,
                      int32_t padding)
{
	OptionItemBase* curr = nullptr;
	bool title_valid = is_title_idx_valid(title_idx);

	assert (action_ && "ERROR: No action function, no player menu.");

	if (player && action_) {
		try {
			curr = new OptionItemPlayer(
			                player, action_,
			                title_valid
			                  ? MenuTitleText[menu_class][menu_lang][title_idx]
			                  : nullptr, // The ctor uses the player name if nullptr.
			                title_idx,
			                menu_y + top, menu_x + left, width, height,
			                padding);
		} catch (std::bad_alloc &e) {
			cerr << __FUNCTION__ << " : failed to allocate new ET_MENU OptionItem\n";
			cerr << " [" << e.what() << "]" << endl;
		}
	}

	return this->insert_option(curr);
}


/** @brief This adds a text option with editable text
  *
  * Please note: The position @a left / @a top are relative to
  * the menu position.
  *
  * Please also note that a title is displayed to the left of the display
  * area.
  *
  * @param[in] target Pointer to the target to handle.
  * @param[in] title_idx Index of the title if it is listed in MenuTitleText.
  * @param[in] max_len Maximum number of characters to allow.
  * @param[in] color Regular display color of the title/target.
  * @param[in] format The format to represent the text, used by snprintf().
  * @param[in] left Relative left position of the display area to the menu.
  * @param[in] top Relative top position of the display area to the menu.
  * @param[in] width Width of the display area. The real width might be larger.
  * @param[in] height Height of the display area.
  * @param[in] padding Distance between title and display.
**/
int32_t Menu::addText(char* target, int32_t title_idx, uint32_t max_len,
                      int32_t color, const char* format,
                      int left, int top, int width, int height, int padding)
{
	OptionItemBase* curr = nullptr;
	bool title_valid = is_title_idx_valid(title_idx);

	assert (title_valid && "ERROR: The given title index is invalid");

	if (target && title_valid) {
		try {
			curr = new OptionItem<char, uint32_t>(
			                    target, max_len, color, ET_TEXT,
			                    MenuTitleText[menu_class][menu_lang][title_idx],
			                    title_idx, format,
			                    menu_y + top, menu_x + left, width, height,
			                    padding);
		} catch (std::bad_alloc &e) {
			cerr << __FUNCTION__ << " : failed to allocate new TEXT OptionItem\n";
			cerr << " [" << e.what() << "]" << endl;
		}
	}

	return this->insert_option(curr);
}


/** @brief This adds an TC_TOGGLE feeding a boolean using a variable title
  *
  * Please note: The position @a left / @a top are relative to
  * the menu position.
  *
  * @param[in] target Pointer to the target to handle.
  * @param[in] title_idx Index of the title if it is listed in MenuTitleText.
  * @param[in] color Regular display color of the title/target.
  * @param[in] left Relative left position of the display area to the menu.
  * @param[in] top Relative top position of the display area to the menu.
  * @param[in] width Width of the display area. The real width might be larger.
  * @param[in] height Height of the display area.
  * @param[in] padding Distance between title and display.
**/
int32_t Menu::addToggle(bool* target, int32_t title_idx, int32_t color,
                        int left, int top, int width, int height, int padding)
{
	OptionItemBase* curr = nullptr;
	bool title_valid = is_title_idx_valid(title_idx);

	assert (title_valid && "ERROR: The given title index is invalid");

	if (target && title_valid) {
		try {
			curr = new OptionItem<bool, uint32_t>(
			                    target, nullptr, ET_TOGGLE,
			                    MenuTitleText[menu_class][menu_lang][title_idx],
			                    title_idx, nullptr, color, TC_NONE,
			                    0, 0, 0, nullptr,
			                    menu_y + top, menu_x + left, width, height,
			                    padding, nullptr);
		} catch (std::bad_alloc &e) {
			cerr << __FUNCTION__ << " : failed to allocate new TOGGLE OptionItem\n";
			cerr << " [" << e.what() << "]" << endl;
		}
	}

	return this->insert_option(curr);
}


/** @brief This adds an TC_TOGGLE feeding a boolean using a fixed title
  *
  * Please note: The position @a left / @a top are relative to
  * the menu position.
  *
  * @param[in] target Pointer to the target to handle.
  * @param[in] title Fixed title to display.
  * @param[in] color Regular display color of the title/target.
  * @param[in] left Relative left position of the display area to the menu.
  * @param[in] top Relative top position of the display area to the menu.
  * @param[in] width Width of the display area. The real width might be larger.
  * @param[in] height Height of the display area.
  * @param[in] padding Distance between title and display.
**/
int32_t Menu::addToggle(bool* target, const char* title_, int32_t color,
                        int left, int top, int width, int height, int padding)
{
	OptionItemBase* curr = nullptr;

	assert (title_ && "ERROR: title_ must be set but is nullptr");

	if (target && title_) {
		try {
			curr = new OptionItem<bool, uint32_t>(
			                    target, nullptr, ET_TOGGLE,
			                    title_, -1, nullptr, color, TC_NONE,
			                    0, 0, 0, nullptr,
			                    menu_y + top, menu_x + left, width, height,
			                    padding, nullptr);
		} catch (std::bad_alloc &e) {
			cerr << __FUNCTION__ << " : failed to allocate new TOGGLE OptionItem\n";
			cerr << " [" << e.what() << "]" << endl;
		}
	}

	return this->insert_option(curr);
}


/** @brief This adds an TC_TOGGLE handling PLAYER::selected
  *
  * Please note: The position @a left / @a top are relative to
  * the menu position.
  *
  * @param[in] player Pointer pointer to the player to handle.
  * @param[in] left Relative left position of the display area to the menu.
  * @param[in] top Relative top position of the display area to the menu.
  * @param[in] width Width of the display area. The real width might be larger.
  * @param[in] height Height of the display area.
  * @param[in] padding Distance between title and display.
**/
int32_t Menu::addToggle(PLAYER** player,
                        int left, int top, int width, int height, int padding)
{
	OptionItemBase* curr = nullptr;

	assert (player && *player
			&& "ERROR: For a player toggle *player must be valid.");

	if (player && *player) {
		try {
			curr = new OptionItemPlayer(player, nullptr,
			                            nullptr, -1,
			                            menu_y + top, menu_x + left,
			                            width, height, padding);
		} catch (std::bad_alloc &e) {
			cerr << __FUNCTION__ << " : failed to allocate new ET_TOGGLE OptionItem\n";
			cerr << " [" << e.what() << "]" << endl;
		}
	}

	return this->insert_option(curr);
}


/// @brief call clear_display(full_display) on all entries
void Menu::clearAll(bool full_clear)
{
	OptionItemBase* curr = root;
	while (curr) {
		curr->clear_display(full_clear);
		curr = curr->getNext();
	}
}


/// @brief return number of menu elements
int32_t Menu::count()
{
	return entry_cnt;
}


/// @brief deletes entry with index @a index
int32_t Menu::delete_entry(int32_t index)
{
	if ( (index >= 0) && (index < entry_cnt) ) {
		OptionItemBase* curr = this->operator[](index);
		if (curr) {
			if (root == curr)
				root = curr->getNext();
			if (tail == curr)
				tail = curr->getPrev();
			delete curr; // This removes it from the list.
			--entry_cnt;
		}
	}

	return entry_cnt;
}


/// @brief call display(full_display) on all entries
void Menu::displayAll(bool full_display)
{
	OptionItemBase* curr = root;
	while (curr) {
		// If a text field (ET_TEXT) is selected, it must be forced
		// to redraw, so the cursor flipping can be in effect:
		if (curr->is_selected() && (ET_TEXT == curr->getType()))
			curr->cursor_flip();

		curr->display(full_display);
		curr = curr->getNext();
	}
}


/** @brief distribute a range of items over specified space
  *
  * This method distributes the items with the index @a first_idx to
  * @a last_idx over columns and rows according to the largest item
  * and available space.
  *
  * @param[in] first_idx The first index to distribute
  * @param[in] last_idx The last index to distribute
  * @param[in] list_width The total width to distribute over if needed
  * @param[in] list_height The total height to distribute over
  * @param[in] y_off Y-Offset where the list starts
  * @param[in] do_update whether to clear the old display or not.
  *
**/
void Menu::distribute(int32_t first_idx, int32_t last_idx,
                      int32_t list_width, int32_t list_height,
                      int32_t y_off, bool do_update)
{
	int32_t item_count   = last_idx - first_idx + 1;
	int32_t item_height  = 0;
	int32_t item_width   = 0;

	// valid?
	assert( ( last_idx >= first_idx)
		  && "ERROR: last_idx must not be smaller than first_idx!");
	assert( ( last_idx < entry_cnt )
		  && "ERROR: last_idx is out of range");
	if ( (last_idx < first_idx) || (last_idx >= entry_cnt) )
		return;

	// 1: Determine minimum width and height:
	int32_t curr_w = 0, curr_h = 0;

	for (int32_t num = first_idx; num <= last_idx; ++num) {
        OptionItemBase* curr = this->operator[](num);
        if (curr) {
			curr->getDimension(curr_w, curr_h);
			if (curr_w > item_width)
				item_width = curr_w;
			if (curr_h > item_height)
				item_height = curr_h;
        }
	}

	// The width must be increased, as items might get selected:
	item_width += select_text_len;

	// Set base values
	int32_t rows     = list_height / item_height;
	int32_t cols     = (item_count / rows)
	                 + (item_count % rows ? 1 : 0);
	int32_t colOff   = (list_width / 2) - (cols * (item_width / 2));

	for (int32_t idx = first_idx; idx <= last_idx; ++idx) {
        OptionItemBase* curr = this->operator[](idx);
        if (curr) {
			int32_t num     = idx - first_idx;
			int32_t cur_col = num / rows;
			int32_t x       = colOff + (cur_col * item_width);
			int32_t y       = item_height * (num % rows);
			curr->move(menu_x + x, menu_y + y_off + y, do_update);
        }
	}
}


/// @brief return pointer to the currently selected entry or nullptr if none is selected
OptionItemBase* Menu::getSelected()
{
	if ( (entry_sel > -1) && (entry_sel < entry_cnt))
		return this->operator[](entry_sel);
	return nullptr;
}


/// @brief return a const pointer to the menu title
const char* Menu::getTitle() const
{
	return title;
}


/// @brief move an entry somewhere else
void Menu::move_entry(int32_t from_idx, int32_t to_idx)
{
	assert( (from_idx > -1) && (from_idx < entry_cnt)
	     && "ERROR: from_idx is out of range!");
	assert( (to_idx > -1) && (to_idx < entry_cnt)
	     && "ERROR: to_idx is out of range!");

	if (from_idx != to_idx) {
		OptionItemBase* toMove = operator[](from_idx);
		assert (toMove && "ERROR: Something is completely FUBAR here!");

		if (to_idx > from_idx)
			// in this case the spot will move one down
			--to_idx;

		// Take it out:
		if (0 == from_idx)
			// It is root
			root = toMove->getNext();
		if ((entry_cnt - 1) == from_idx)
			// Or tail
			tail = toMove->getPrev();
		toMove->remove();
		--entry_cnt;

		// And re-insert
		if (0 == to_idx) {
			toMove->insert_before(root);
			root = toMove;
		} else if (entry_cnt == to_idx) {
			toMove->insert_after(tail);
			tail = toMove;
		} else
			toMove->insert_before(this->operator[](to_idx));
		++entry_cnt;
	} // end of to_idx != from_idx
}


/// @brief do a redraw of one element
void Menu::redraw(int32_t index, bool update_full)
{
	if ( (index >= 0) && (index < entry_cnt) ) {
		OptionItemBase* curr = this->operator[](index);
		if (curr) {
			curr->clear_display(update_full);
			curr->display(update_full);
		}
	}
}


/// @brief do a full redraw of everything
void Menu::redrawAll(bool full_redraw)
{
	SHOW_MOUSE(nullptr)
	this->clearAll(full_redraw);

	// If this is a full redraw, the background and
	// menu title must be drawn as well.
	if (full_redraw) {
		if (++bgOffset == INT_MAX)
			bgOffset = 0;
		drawMenuBackground (bgType, bgOffset, bgItems);
		textout_ex (global.canvas, font, title, title_x + 2, menu_y + 12, BLACK, -1);
		textout_ex (global.canvas, font, title, title_x + 5, menu_y + 14, WHITE, -1);
	}

	this->displayAll(full_redraw);
	SHOW_MOUSE(global.canvas)

	if (full_redraw)
		quickChange(false);

}


void Menu::setLanguage(bool autorefresh)
{
	if (env.language != menu_lang) {
		menu_lang = env.language;

		if (!title_set)
			title = MenuTitleText[menu_class][menu_lang][0];

		OptionItemBase*    curr   = root;
		const char* const* titles = MenuTitleText[menu_class][menu_lang];

		while (curr) {
			int32_t title_idx = curr->getTitleIdx();

			// 1: Set new title (if not manually set)
			if (title_idx > -1)
				curr->setTitle(titles[title_idx]);

			// 2: Set new text array if based on a pre-set
			if (curr->needs_text()) {
				const char* const* texts = OptionClassText[curr->getTextClass()][menu_lang];
				curr->setTexts(const_cast<const char**>(texts));
			}

			// 3: If this is a sub-menu, call an update dispatcher
			if (ET_MENU == curr->getType())
				static_cast<OptionItemMenu*>(curr)->setLanguage();

			curr = curr->getNext();
		}

		if (autorefresh)
			this->redrawAll(true);
	}
}


void Menu::setTitle(const char* new_title, bool autorefresh)
{
	if (new_title) {
		// Delete old title if it was set already
		if (title_set && title)
			free(const_cast<char*>(title));

		title = strdup(new_title);
		title_set = true;

		if (autorefresh)
			this->redrawAll(true);
	}
}


/* ------------------------
 * --- Public operators ---
 * ------------------------
 */


/** @brief Parentheses operator to use an instance like a function
  *
  * The operator hands over input control to the menu. It will return only
  * if an option is a button that returns a key code. So make sure to have
  * at least one button per menu that lets you get out ... or stay forever. ;)
  *
  * <I>Note</I>: As a safety measure the operator checks for the existence of
  * a returning button and asserts that existence. New menus should be checked
  * in debug mode at least once.
  *
  * @return The key code of a clicked exiting button.
**/
int32_t Menu::operator()()
{
	bool has_exit_button = false;
	OptionItemBase* curr = root;

	while (!has_exit_button && curr) {
		has_exit_button = curr->isExitButton();
		curr = curr->getNext();
	}

	assert(has_exit_button && "ERROR: A Menu without an exit button is unleavable!");

	// Needed Loop values :
	int32_t key_code        = -1;
	int32_t end_event       = 0;
	int32_t allegro_key     = 0;
	bool    mlb_is_pressed  = false; // Left mouse button is pressed,
	bool    mlb_is_released = true;  // and was released.
	bool    mrb_is_pressed  = false; // Right mouse button is pressed,
	bool    mrb_is_released = true;  // and was released.
	int32_t ms_per_frame    = 1000 / env.frames_per_second;
	int32_t mlb_x           = 0;
	int32_t mlb_y           = 0;
	int32_t mouse_clock     = MOUSE_RELEASE_DELAY;
	int32_t mouse_round     = 0;
	int32_t mouse_reduct    = 0;
	bool    has_ctrl_down   = false;
	eEntryType last_clicked = ET_NONE;

	flush_inputs();
	WIN_CLOCK_INIT
	menu_ms_reset();

	// Set background style
	bgType   = env.dynamicMenuBg
	         ? static_cast<eBackgroundTypes>(rand() % BACKGROUND_COUNT)
	         : BACKGROUND_BLANK;
	bgOffset = (RAND_MAX / 4) + (rand() % (RAND_MAX / 4));
	bgItems  = (rand() % 100) + 20;

	// Initial display:
	redrawAll(true);

	/* ---------------------------------------
	 * --- Input handling and drawing loop ---
	 * ---------------------------------------
	 */
	while (-1 == key_code) {
		int32_t ms_unused = ms_per_frame - menu_ms_get();
		if (ms_unused > 0)
			MSLEEP(ms_unused)
		redrawAll(true);

		if (global.isCloseBtnPressed()) {
			key_code  = KEY_ESC;  // Exit loop
			end_event = key_code; // Exit menu
			continue;
		}

		/// --------------------------------------
		/// --- A) Pre-handle key press events ---
		/// --------------------------------------
		has_ctrl_down = (key[KEY_LCONTROL] || key[KEY_RCONTROL]);

		if ( keypressed() ) {
			allegro_key = readkey();
			key_code = allegro_key >> 8;
			if (KEY_DOWN == key_code) {
				this->selectNext();
				key_code = -1;
			} else if (KEY_UP == key_code) {
				this->selectPrev();
				key_code = -1;
			} else if (KEY_ENTER_PAD == key_code)
				key_code = KEY_ENTER;

		} // End of having a pressed key


		/// --------------------------------------
		/// --- B) Handle mouse button events  ---
		/// --------------------------------------

		mlb_x = mouse_x;
		mlb_y = mouse_y;

		// Set mouse button status anew
		mlb_is_pressed = mouse_b & 1 ? true : false;
		mrb_is_pressed = mouse_b & 2 ? true : false;

		// Fix release status on mouse button states:
		if (!mlb_is_pressed) mlb_is_released = true;
		if (!mrb_is_pressed) mrb_is_released = true;

		// reset mouse clock if both are released
		if (mlb_is_released && mrb_is_released) {
			mouse_clock  = MOUSE_RELEASE_DELAY;
			mouse_round  = 0;
			mouse_reduct = 0;
		}

		// Be sure only new left mouse button presses are recorded
		if (mlb_is_released && mlb_is_pressed)
			mlb_is_released = false;
		else if (mlb_is_pressed) {
			if ( (--mouse_clock > 0)
			  || ( (ET_VALUE != last_clicked)
				&& (ET_COLOR != last_clicked) ) )
				mlb_is_pressed = false;
		}

		// The same applies to the right button
		if (mrb_is_released && mrb_is_pressed && !mlb_is_pressed)
			mrb_is_released = false;
			// Note: But the release is set to false anyway, so pressing
			// both buttons will not result in a right mouse button event
			// if held and the left button is released.
		else if (mrb_is_pressed) {
			if ( (--mouse_clock > 0)
			  || ( (ET_VALUE != last_clicked)
				&& (ET_COLOR != last_clicked) ) )
				mrb_is_pressed = false;
		}

		// Handle the mouse delay:
		if (!mouse_clock) {
			if ( (ET_VALUE == last_clicked)
			  || (ET_COLOR == last_clicked) ) {
				if (MOUSE_DELAY_REDUCT == ++mouse_round) {
					mouse_round = 0;
					if ( (ET_COLOR == last_clicked)
					  || (++mouse_reduct >= MOUSE_RELEASE_DELAY) )
						mouse_reduct = MOUSE_RELEASE_DELAY - 1;
				}
			} else
				mouse_reduct = 0;
			mouse_clock = MOUSE_RELEASE_DELAY - mouse_reduct;
		}

		// Determine whether a click hit something
		int32_t event = mlb_is_pressed || mrb_is_pressed
		              ? selectClicked(mlb_x, mlb_y)
		              : 0;


		/// --------------------------------
		/// --- C) Handle current events ---
		/// --------------------------------

		if ( event || (key_code > 0) ) {
			curr = getSelected();
			if (curr) {
				eEntryType type      = curr->getType();
				bool       old_mouse = env.osMouse; // To catch mouse changes

				// Note whether clicked on elements for the clock delay reduction
				if (event)
					last_clicked = type;
				else
					last_clicked = ET_NONE;

				// ET_VALUE needs handling for left/right keys:
				if (ET_VALUE == type) {
					if (KEY_RIGHT == key_code)
						event = 1;
					else if (KEY_LEFT == key_code)
						event = -1;
					// If the right mouse button or ctrl key was pressed,
					// multiply the event by 10
					if (mrb_is_pressed || has_ctrl_down)
						event *= 10;
				}

				// Set key_code to activation result
				key_code = curr->activate(event, mlb_x, mlb_y, allegro_key);

				// If this was a sub menu, redraw the current menu:
				if (ET_MENU == type)
					redrawAll(true);

				// Some elements trigger end-events
				if ( (key_code > 0)
					// If this was a sub menu with KEY_ESC result,
					// it just means that the sub menu was closes:
				  && ( (ET_MENU != type)
				    || (KEY_ESC != key_code) ) ) {
					end_event = key_code;
				} else
					key_code = -1;
				allegro_key = 0;

				// If the mouse was changed, the change must be performed
				// at once. If we didn't do this here, switching back to
				// standard mouse makes it invisible until the menu exits.
				if (old_mouse != env.osMouse)
					init_mouse_cursor();

			} // End of having a menu entry to handle
		} // End of having mouse button or key event

		// Update non-OS mouse movements
		SHOW_MOUSE(global.canvas)

	} // End of input/drawing loop

	WIN_CLOCK_REMOVE

	// As the menu does not clear error messages, a possible
	// message must be cleared now:
	if (errorMessage)
		errorMessage = nullptr;

	return end_event;

} // End of Menu::operator()()


/** @brief Get a stored option by index
  *
  * If @a index is out of range, nullptr is returned.
  *
  * @param[in] index The index of the wanted option, starting with 0.
**/
OptionItemBase* Menu::operator[](int32_t index)
{
	OptionItemBase* result = nullptr;

	if ((-1 < index) && (entry_cnt > index)) {
		int32_t cur_idx = 0;
		bool    go_up   = true;
		result          = root;

		// Start front or end?
		if (index > (entry_cnt / 2)) {
			result  = tail;
			cur_idx = entry_cnt - 1;
			go_up   = false;
		}

		// Just wander, this should be safe.
		while (result && (cur_idx != index)) {
			if (go_up) {
				result = result->getNext();
				++cur_idx;
			} else {
				result = result->getPrev();
				--cur_idx;
			}

			assert(result && "ERROR: Something is wrong with the list!");
		}
	} // End of having a sane index

	return result;
}


/* -----------------------
 * --- Private methods ---
 * -----------------------
 */

/// @brief simple singly list insert
int32_t Menu::insert_option(OptionItemBase* new_opt)
{
	if (new_opt) {
		// Insert into list:
		if (tail) {
			new_opt->insert_after(tail);
			tail = new_opt;
		} else {
			root = new_opt;
			tail = new_opt;
		}

		++entry_cnt;
	}
	return entry_cnt;
}


/// @brief simple singly list insert with title setting
int32_t Menu::insert_option(OptionItemBase* new_opt, int32_t title_idx,
							const char* title_)
{
	if (new_opt) {
		if (title_)
			new_opt->setTitle(title_);
		else if (is_title_idx_valid(title_idx))
			new_opt->setTitle(MenuTitleText[menu_class][menu_lang][title_idx]);
		return insert_option(new_opt);
	}
	return entry_cnt;
}


/// @brief return true if @a title_idx is lower than the first 0x0 entry
bool Menu::is_title_idx_valid(int32_t title_idx)
{
	int32_t curr_idx = 0;
	const char* const* titles = MenuTitleText[menu_class][menu_lang];

	while ( (curr_idx < title_idx) && titles[curr_idx] )
		++curr_idx;

	return ( (title_idx > -1) && (curr_idx == title_idx) && titles[curr_idx]);
}


/** @brief This method selects the entry under the mouse.
  *
  * If the mouse position, described by the @a x and @a y parameters,
  * is not over any entry, nothing happens and 0 is returned.
  *
  * If the activated entry is an ET_VALUE and one of the change buttons is hit,
  * the method returns -1 for down and +1 for up.
  *
  * If the activated entry is an ET_BUTTON with associated key code, the key
  * code is returned.
  *
  * In all other cases 0 is returned, as clicking must be activated manually.
  *
  * @param[in] x Mouse x coordinate.
  * @param[in] y Mouse y coordinate.
  * @return -1/+1 for ET_VALUE change buttons, associated key code for ET_BUTTON
  * and 0 in all other cases.
**/
int32_t Menu::selectClicked(int32_t x, int32_t y)
{
	OptionItemBase* curr     = root;
	OptionItemBase* result   = nullptr;
	int32_t         retval   = 0;
	int32_t         curr_idx = -1;

	while (curr && !result) {
		++curr_idx;
		if (curr->is_click_in(x, y, retval))
			result = curr;
		else
			curr = curr->getNext();
	}

	if (result && !result->is_selected()) {
		unselect();
		entry_sel = curr_idx;
		result->select();
	}

	return retval;
}


/** @brief unselect current selected entry (if any) and select the next.
  * If no entry is selected, the first one will be chosen.
  * If the last entry is selected, no entry will be chosen.
**/
void Menu::selectNext()
{
	OptionItemBase* curr = nullptr;

	if (entry_sel > -1) {
		curr = operator[](entry_sel);
		if (curr)
			curr->unselect();
	}

	// If this was the last entry, none is to be selected
	if (++entry_sel >= entry_cnt)
		entry_sel = -1;
	else {
		if (curr)
			// The previous was unselected
			curr = curr->getNext();
		else
			curr = operator[](entry_sel);

		// Be sure the container works properly!
		assert(curr && "ERROR: Something is wrong with the OptionEntry list!");

		if (curr)
			curr->select();
	}
}


/** @brief unselect current selected entry (if any) and select the previous.
  * If no entry is selected, the first one will be chosen.
  * If the last entry is selected, no entry will be chosen.
**/
void Menu::selectPrev()
{
	OptionItemBase* curr = nullptr;

	if (entry_sel > -1) {
		curr = operator[](entry_sel);
		if (curr)
			curr->unselect();
	}

	// If this was the first entry, none is to be selected
	if (--entry_sel != -1) {
		// Rotate to the end if none was selected
		if (entry_sel < -1)
			entry_sel = entry_cnt - 1;

		if (curr)
			// The next was unselected
			curr = curr->getPrev();
		else
			curr = operator[](entry_sel);

		// Be sure the container works properly!
		assert(curr && "ERROR: Something is wrong with the OptionEntry list!");

		if (curr)
			curr->select();
	}
}


/// @brief little helper to be able to add options from inside the header
void Menu::setTexts(OptionItemBase* item,
                    const char** texts,
                    eTextClass text_class)
{
	assert(  item && (texts || (TC_FREETEXT != text_class))
	      && (TC_NONE != text_class) && "ERROR: This does not fit at all!");
    if (item) {
		if ( (TC_FREETEXT == text_class) && texts) {
			item->setTextClass(text_class);
			item->setTexts(texts);
		} else if (TC_NONE != TC_FREETEXT) {
			item->setTextClass(text_class);
			item->setTexts(const_cast<const char**>(OptionClassText[text_class][menu_lang]));
		}
    }
}


/** @brief unselect current selected entry (if any).
  * If no entry is selected, nothing happens.
**/
void Menu::unselect()
{
	OptionItemBase* curr = nullptr;

	if (entry_sel > -1) {
		curr = operator[](entry_sel);
		if (curr)
			curr->unselect();
		entry_sel = -1;
	}
}


/// @brief display function for tank bitmaps plus tank type text
bool display_tank_desc(int32_t* tanknum, int32_t x, int32_t y)
{
	assert(tanknum && "ERROR: tanknum must be set");

	assert(*tanknum > -1 && "ERROR: tanknum too low");
	assert(*tanknum < TT_TANK_COUNT && "ERROR: tanknum too high");

	if (!tanknum || (*tanknum < 0) || (*tanknum >= TT_TANK_COUNT))
		return false;

	BITMAP*     tank_bmp   = env.tank[   *tanknum ? *tanknum + TO_TANK   : *tanknum];
	BITMAP*     turr_bmp   = env.tankgun[*tanknum ? *tanknum + TO_TURRET : *tanknum];
	int32_t     tank_off_x = ROUNDu(tank_bmp->w / 2);
	int32_t     tank_off_y =        tank_bmp->h;
	int32_t     turr_off_x = ROUNDu(turr_bmp->w / 2);
	int32_t     turr_off_y = ROUNDu(turr_bmp->h / 2) - 2;
	int32_t     tank_x     = x + tank_off_x + 1;
	int32_t     tank_y     = y + turr_off_y + 1;
	int32_t     text_y     = tank_y + (tank_off_y / 2) - (env.fontHeight / 2);
	int32_t     text_x     = tank_x + tank_off_x + 5;
	const char* tank_text  = OptionClassText[TC_TANKTYPE][env.language][*tanknum];

	draw_sprite   (global.canvas, tank_bmp, tank_x - tank_off_x, tank_y);
	rotate_sprite (global.canvas, turr_bmp, tank_x - turr_off_x, tank_y - turr_off_y,
	               itofix(224) );

	textout_ex (global.canvas, font, tank_text, text_x, text_y, BLACK, -1);

	global.make_update(x, y, text_x + text_length(font, tank_text) - x,
	                   tank_y + turr_off_y + tank_off_y - y);

	return true;
}

