#include "button.h"
#include "menu.h"
#include "optionitembase.h"
#include "floattext.h"

static const char menu_hint_text[]   = "-> ";
static uint32_t   menu_hint_text_len = 0; // Set by ctor when "font" is set
static const char select_text[]      = "* ";
uint32_t          select_text_len    = 0; // Set by ctor when "font" is set

// Note: Direct setting is not a good idea, font might be anything when
//       the static initialization is done.
static int32_t CURSOR_FLIP_TIME = 25; // Delays cursor flipping


/** @brief default and only constructor
  * @param[in] type_ Type of the option.
  * @param[in] title_ The title of the option to display.
  * @param[in] titleIdx_ Index value of the submitted title. -1 means @a title_ is fixed.
  * @param[in] text_ Array of texts to display.
  * @param[in] color_ The color to use for the text, mainly useful for OT_TOGGLE.
  * @param[in] class_ Index value of the submitted texts or TC_NONE if no texts are needed.
  * @param[in] format_ Format string to use when displaying the ET_TEXT target.
  * @param[in] top_ Top position of the display area.
  * @param[in] left_ Left position of the display area.
  * @param[in] width_ Width of the display area.
  * @param[in] height_ Height of the display area.
  * @param[in] padding_ Padding of the title and buttons to the display area.
  * @param[in] show_size_ Sets the size of the show color box. (ET_COLOR only)
**/
OptionItemBase::OptionItemBase( eEntryType    type_,
                                const char*   title_,
                                int32_t       titleIdx_,
                                const char**  text_,
                                int32_t       color_,
                                eTextClass    class_,
                                const char*   format_,
                                int32_t top_, int32_t left_,
                                int32_t width_, int32_t height_,
                                int32_t padding_, int32_t show_size_) :
	color(color_),
	format(format_),
	height(height_),
	left(left_),
	padding(padding_),
	show_size(show_size_),
	textClass(class_),
	texts(text_),
	title(title_),
	titleIdx(titleIdx_),
	top(top_),
	type(type_),
	width(width_)
{
	// Assert the title as the most basic value
	assert (title && "Title not set");
	assert ((texts || (TC_NONE == textClass)) && "text_ and class_ do not fit!");
	titleLen = text_length(font, title);

    // Set static globals
    if (0 == menu_hint_text_len)
		menu_hint_text_len = text_length(font, menu_hint_text);
    if (0 == select_text_len)
		select_text_len = text_length(font, select_text);
}


/// @brief simple default destructor
OptionItemBase::~OptionItemBase()
{
	this->remove();
	if (this->button)
		delete this->button;
}


/* =====================================
 * === Public method implementations ===
 * =====================================
 */

/** @brief clear the current display
  *
  * This method erases the current display. It should be called
  * before changing the displayed value or the display parameters
  * like coordinates or dimension.
  *
  * The methods that change those parameters or draw the display call
  * clear_display() automatically. Please remember to call it in external
  * display functions.
  *
  * Note: As ET_TOGGLE have their title being the text to display, setting
  * @a update_full will only change the state of the decorated state and not
  * clear any additional space.
  */
void OptionItemBase::clear_display(bool update_full)
{
	if (drawn || (update_full && decorated)) {
		int32_t xLeft   = left;
		int32_t xWidth  = drawn ? width : 0;
		int32_t xTop    = top;
		int32_t xHeight = height;

		// clear decoration?
		if (update_full) {

			// First: Title if displayed left of the display area
			if ((ET_TOGGLE != type) && (ET_BUTTON != type)) {
				xLeft  -= titleLen + padding;
				xWidth += titleLen + padding;
			}

			// Second: Selection text if needed
			if (selected) {
				if ( (ET_BUTTON == type) && this->button ) {
					// The button box must be erased
					xLeft   -= 3;
					xTop    -= 3;
					xWidth  += 6;
					xHeight += 6;
				} else {
					xLeft  -= select_text_len + padding;
					xWidth += select_text_len + padding;
				}
			}

			// Third: The ET_VALUE wheel buttons
			if (ET_VALUE == type)
				xWidth += 2 * padding + 20;
		}

		rectfill (global.canvas, xLeft, xTop, xLeft + xWidth, xTop + xHeight,
		          makecol (0,79,0));
		global.make_update (xLeft, xTop, xWidth, xHeight);
		drawn = false;
		if (update_full)
			decorated = false;
	}
}


/// @brief Toggle cursor_on if this is a selected ET_TEXT
void OptionItemBase::cursor_flip()
{
	if ((ET_TEXT == type) && selected && !read_only) {
		if (--curs_clk < 1) {
			curs_clk  = CURSOR_FLIP_TIME;
			cursor_on = !cursor_on;
			clear_display(false);
		}
	}
}


/// @brief get width and height at once
/// Note: Special circumstances like padding, menu indication and
///       ET_TOGGLE extra sizes are added.
void OptionItemBase::getDimension (int32_t &tgt_width, int32_t &tgt_height)
{
	tgt_width  = width + padding;
	tgt_height = height + padding + 2;

	if (ET_COLOR == type)
		tgt_width += padding + show_size;
	if (ET_MENU == type)
		tgt_width += menu_hint_text_len;
	if (ET_TOGGLE == type) {
		tgt_width  += 4;
		tgt_height += 2;
	}
}

/// @brief return currently set key code
int32_t OptionItemBase::getKeyCode()
{
	return keyCode;
}


/// @brief return the value of the next pointer
OptionItemBase* OptionItemBase::getNext()
{
	return next;
}


/// @brief return the value of the prev pointer
OptionItemBase* OptionItemBase::getPrev()
{
	return prev;
}


/// @brief return text class as index value
uint32_t OptionItemBase::getTextClass()
{
	return static_cast<uint32_t>(textClass);
}


/// @brief return the index value of the displayed title
uint32_t OptionItemBase::getTitleIdx()
{
	return titleIdx;
}


/// @brief return the eEntryType of the entry
eEntryType OptionItemBase::getType()
{
	return type;
}


/** @brief Insert option after @a new_prev
  *
  * This is a standard insert into a doubly linked list.
  *
  * @param[in,out] new_prev (Optional) pointer to the option that becomes the new prev.
  */
void OptionItemBase::insert_after(OptionItemBase* new_prev)
{
	if (prev || next)
		this->remove();

	prev = new_prev;
	if (prev) {
		next = prev->next;
		prev->next = this;
	}

	if (next)
		next->prev = this;
}


/** @brief Insert option before @a new_next
  *
  * This is a standard insert into a doubly linked list.
  *
  * @param[in,out] new_next (Optional) pointer to the option that becomes the new next.
  */
void OptionItemBase::insert_before(OptionItemBase* new_next)
{
	if (prev || next)
		this->remove();

	next = new_next;
	if (next) {
		prev = next->prev;
		next->prev = this;
	}

	if (prev)
		prev->next = this;
}


/** @brief return true if @a x and @a y are in this options clickable area
  *
  * This method returns true if @a x and @a y are with the clickable area
  * of this option. This means the display area and optional wheel buttons
  * if this is an ET_VALUE type.
  *
  * If this option is an ET_BUTTON and has a key code assigned, or if this is
  * an ET_VALUE and the wheel buttons are hit, @a ret is set to the appropriate
  * value.
  *
  * @param[in] x X coordinate to test.
  * @param[in] y Y coordinate to test.
  * @param[out] ret Value to set to an assigned key code or -1/+1 for ET_VALUE inc/dec click.
  * @return true if this option is hit, false otherwise.
**/
bool OptionItemBase::is_click_in(int32_t x, int32_t y, int32_t &ret)
{
	int32_t xLeft   = left  + 1;
	int32_t xTop    = top   + 1;
	int32_t xRight  = xLeft + width  - 2;
	int32_t xBottom = xTop  + height - 2;
	bool    result  = false;

	// reset ret
	ret = 0;

	// Note: No need to check anything if y is somewhere else
	if ( (y >= xTop ) && (y <= xBottom) ) {
		bool hasWheelresult = false;

		// Check direct display area:
		if ( (x >= xLeft) && (x <= xRight ) )
			result = true;

		// If this is an ET_VALUE, check wheel buttons
		if (!result && (ET_VALUE == type)) {
			int32_t up_left  = left + width + padding;
			int32_t up_right = up_left + 10;
			int32_t dn_left  = up_right + padding;
			int32_t dn_right = dn_left + 10;

			// Left "DOWN" button
			if ( (x >= up_left) && (x <= up_right) ) {
				result = true;
				ret    = -1;
				hasWheelresult = true;
			}

			// Right "UP" button
			else if ( ( x >= dn_left) && (x <= dn_right) ) {
				result = true;
				ret    = 1;
				hasWheelresult = true;
			}
		} // End of checking ET_VALUE wheel buttons

		// Is there a return code to send?
		if (result && !hasWheelresult)
			// simply activate it
			ret = KEY_ENTER;

		// If a result is found, this must be updated:
		if (result && (ET_COLOR != type))
			this->clear_display(false);
	} // End of y in range

	return result;
}


/// @brief returns true if this entry is selected
bool OptionItemBase::is_selected()
{
	return selected;
}


/** @brief move the display area
  *
  * This method clears the current display and then changes the position of the
  * display area of this option.
  *
  * @param[in] new_left The new left position of the display area.
  * @param[in] new_top The new top position of the display area.
  * @param[in] do_update if set to true, the current display is cleared.
  */
void OptionItemBase::move(int32_t new_left, int32_t new_top, bool do_update)
{
	if ((new_left != left) || (new_top != top)) {
		if (do_update)
			clear_display(true);
		left = new_left;
		top  = new_top;
	}
}


/// @brief return true if this option needs set texts
bool OptionItemBase::needs_text()
{
	return (textClass < TC_TEXTCLASS_COUNT);
}


/** @brief remove this option from the list.
  *
  * This is a standard remove from a doubly linked list.
  */
void OptionItemBase::remove()
{
	if (next)
		next->prev = prev;
	if (prev)
		prev->next = next;
	prev = nullptr;
	next = nullptr;
}


/** @brief resize the display area
  *
  * This method clears the current display and then changes the dimensions of
  * the display area of this option.
  *
  * @param[in] new_width The new width of the display area.
  * @param[in] new_height The new height of the display area.
  */
void OptionItemBase::resize(int32_t new_width, int32_t new_height)
{
	if ((new_width != width) || (new_height != height)) {
		clear_display(true);
		width  = new_width;
		height = new_height;
	}
}


/// @brief selects this entry and triggers a redraw
void OptionItemBase::select()
{
	if (!selected) {
		selected = true;
		clear_display(true);
	}
}


/** @brief Change the padding around the display area
  *
  * This method clears the current display and then changes the padding around
  * the display area of this option.
  *
  * @param[in] new_padding The new padding value.
  */
void OptionItemBase::setPadding(int32_t new_padding)
{
	if (new_padding != padding) {
		clear_display(true);
		padding = new_padding;
	}
}


/** @brief set a new title
  *
  * This method clears the current display and then changes the title. Use this
  * to switch languages.
  *
  * @param[in] new_title Pointer to the new title to display
  */
void OptionItemBase::setTitle(const char* new_title)
{
	if (new_title != title) {
		clear_display(true);
        title    = new_title;
		titleLen = text_length(font, title);
		if ((ET_BUTTON == type) && button)
			button->setText(title);
	}
}


/** @brief set a new text class
  *
  * The only reaal purpose is to be able to add text-less
  * options in templated add methods in the Menu class,
  * where the OptionClassText is not available.
  *
  * Call this then from the compilation unit.
**/
void OptionItemBase::setTextClass (eTextClass new_class)
{
	textClass = new_class;
}


/** @brief set new texts array
  *
  * This method clears the current display without decorations and changes the
  * used texts array. Use this to switch languages.
  */
void OptionItemBase::setTexts(const char** new_texts)
{
	if (new_texts != texts) {
		clear_display(false);
        texts = new_texts;
	}
}


/// @brief unselects this entry and triggers a redraw
void OptionItemBase::unselect()
{
	if (selected) {
		clear_display(true);
		selected  = false;
		cursor_on = false;
		curs_clk  = CURSOR_FLIP_TIME;
	}
}


/* ========================================
 * === Protected method implementations ===
 * ========================================
 */


/** @brief return the outcome of a menu activation
  * @param[in] target pointer to the menu
  * @return Result of Menu::operator()
**/
int32_t OptionItemBase::activateMenu(Menu* target)
{
	if (target)
		return target->operator()();
	return 0;
}


/** @brief Add the result of key press @a k to @a target.
  *
  * Please make sure that @a has at least one byte free space excluding
  * null character termination!
  *
  * @param[out] target The char array (or string) to receive the result.
  * @param[in] k Allegro 4 raw key code, or the allegro 5 unichar field
  */
void OptionItemBase::activateText(char* target, int32_t k)
{
	if (target && (k > 0) ) {
		int32_t oldTextLen = static_cast<int32_t>(strlen(target));
		char    chr        = static_cast<char>(k & 0xff);
		textLen = oldTextLen;

		if ( ((0x08 == chr) || (0x7f == chr)) && textLen )
			target[--textLen] = 0x0;
		else if (isprint(chr))
			target[textLen++] = chr;

		if (oldTextLen != textLen)
			clear_display(false);
	}
}


/** @brief Switch the state of @a target
  *
  * @param[in,out] target The target value to switch
  */
void OptionItemBase::activateToggle(bool* target)
{
	if (target) {
		*target = !(*target);
		clear_display(false);
	}
}


/** @brief display the set button
**/
void OptionItemBase::displayButton()
{
	if (!drawn) {
		if (this->button)
			button->draw();
		drawn = true;
	}
}


/** @brief Display the options decoration (title plus button(s))
  *
  * Note: If this is ET_TOGGLE, the method will only change the decorated
  * state to true, as the title for this type is the displayed text.
  *
  * @param[in] show_color Used only for the ET_COLOR show box.
  */
void OptionItemBase::displayDeco(int32_t show_color)
{
	if (!decorated) {
		int32_t xLeft    = left;
		int32_t xWidth   = 0;
		int32_t xTop     = top;
		int32_t xHeight  = height;
		int32_t text_top = top + (height / 2) - (env.fontHeight / 2);
		int32_t deco_top = text_top + 4;

		// First: Title text
		if (title
		  && (ET_BUTTON != type)
		  && (ET_MENU   != type)
		  && (ET_TOGGLE != type)) {
		  	int32_t tColor = (ET_COLOR == type) || textOnly ? BLACK : color;

			xLeft -= titleLen + padding;
			xWidth = titleLen + padding;

			// Add a nice shadow if wanted
		  	if (env.shadowedText)
				textout_ex (global.canvas, font, title, xLeft + 1, text_top + 1,
								GetShadeColor(tColor, true, PINK), -1);

			textout_ex (global.canvas, font, title, xLeft, text_top, tColor, -1);
		}

		// ET_MENU has an additional "-> " displayed next to it
		if ( (ET_MENU == type) && show_menu) {
			xLeft  -= menu_hint_text_len + padding;
			xWidth += menu_hint_text_len + padding;
			textout_ex (global.canvas, font, menu_hint_text, xLeft, text_top, BLACK, -1);
		}

		// Second: Selection text if needed
		if (selected) {
			if ( (ET_BUTTON == type) && this->button ) {
				// Buttons get a box drawn around them
				xTop    -= 3;
				xLeft   -= 3;
				xWidth  += 6 + width; // spans over
				xHeight += 6;
				rect (global.canvas, xLeft, xTop, xLeft + xWidth, xTop + xHeight, WHITE);
			} else  {
				xLeft  -= select_text_len + padding;
				xWidth += select_text_len + padding;
				textout_ex (global.canvas, font, select_text, xLeft, text_top, BLACK, -1);
			}
		}

		// If an actual width could be determined, add an update
		// for the text region:
		if (xWidth > 0)
			global.make_update (xLeft, xTop, xWidth, xHeight);

		// Third: The ET_VALUE wheel buttons / ET_COLOR display box
		if (ET_VALUE == type) {
			BITMAP* arrow = env.misc[6];
			int32_t dn_left = left + width + padding;
			int32_t up_left = dn_left + padding + 10;

			if (this->canGoDown())
				draw_sprite_v_flip (global.canvas, arrow, dn_left, deco_top);
			if (this->canGoUp())
				draw_sprite        (global.canvas, arrow, up_left, deco_top);

			global.make_update (up_left, top, 10, height);
		} else if (ET_COLOR == type) {
			int32_t x = left + width + padding;

			rect (global.canvas, x, deco_top, x + show_size, deco_top + show_size, BLACK);
			rectfill (global.canvas,
					x + 1, deco_top + 1,
					x + show_size - 1, deco_top + show_size - 1,
					show_color);
			global.make_update (x, deco_top, show_size, show_size);
		}

		decorated = true;
	}
}


/** @brief Display the @a target  menu title as text.
  *
  * @param[in] target pointer to the menu to display
  */
void OptionItemBase::displayMenu(Menu* target)
{
	if (title && (titleIdx > -1))
		this->displayText(title);
	else if (target)
		this->displayText(target->getTitle());
	else
		this->displayText("Oh no! No Menu Title!");
}


/** @brief Display the @a target as text.
  *
  * @param[in] target (optional) pointer to the text to display
  */
void OptionItemBase::displayText(char* target)
{
	this->displayText(static_cast<const char*>(target));
}


/** @brief Display the @a target as text.
  *
  * @param[in] target (optional) pointer to the text to display
  */
void OptionItemBase::displayText(const char* target)
{
	if (!drawn) {
		if (!textOnly) {
			rect (global.canvas, left, top, left + width, top + height, BLACK);
			rectfill (global.canvas,
						left + 1, top + 1,
						left + width - 1, top + height - 1,
						WHITE);
		}

		// First display the text
		const char* txt_p = target;
		int32_t txt_len   = txt_p ? text_length(font, txt_p) : 0;

		if (txt_p) {
			int32_t len_available = width - 6;
			int32_t text_top = top + (height / 2) - (env.fontHeight / 2);


			// If this is a read/write selected ET_TEXT, it has a cursor being
			// drawn, and thus less space to display the text:
			if ((ET_TEXT == type) && selected && !read_only)
				len_available -= text_length(font, "W");

			// Scroll text until it fits.
			while (txt_p && txt_p[0] && (txt_len > len_available))
				txt_len = text_length(font, ++txt_p);

			// Now print the result
			if (txt_p && txt_p[0]) {

				// With a shadow? But not in text field mode
				if (env.shadowedText && textOnly)
					textout_ex (global.canvas, font, txt_p, left + 4, text_top + 1,
					            GetShadeColor(color, true, PINK), -1);

				textout_ex (global.canvas, font, txt_p, left + 3, text_top,
				            textOnly ? color : BLACK, -1);
			}
		}

		// Then a cursor if turned on
		if (cursor_on) {
			int32_t offset = txt_len + left + 3;
			rectfill (global.canvas, offset, top + 3, offset + 6, top + height - 3, BLACK);
		}

		global.make_update (left, top, width, height);
		drawn = true;
	}
}


/** @brief Display the @a target as text.
  *
  * @param[in] target (optional) pointer to the text to display
  */
void OptionItemBase::displayText(uint32_t* target)
{
	this->displayValue(target);
}


/** @brief Displays a toggle option in either black on white or vice versa
  *
  * Note: The title is actually the displayed text.
  *
  * @param[in] target (optional) pointer to the toggle option.
  * to be set by OptionItemPlayer only.
  */
void OptionItemBase::displayToggle(bool* target)
{
	if (!drawn) {
		int32_t fg_color = color;
		int32_t bg_color = BLACK;
		int32_t sh_color = DARK_GREY;
		int32_t x_radius = (titleLen / 1.8) + padding + 5;
		int32_t y_radius = height / 2;
		int32_t xLeft    = left + (width / 2);
		int32_t xTop     = top  + (height / 2);
		int32_t txtLeft  = xLeft - (titleLen / 2) + 1;

		// Swap if activated
		if (target && *target) {
			fg_color = BLACK;
			bg_color = color;
			sh_color = GREY;
		}

		// To make this nicer looking, a shade color is generated for an outline:
		ellipsefill (global.canvas, xLeft, xTop, x_radius, y_radius, bg_color);
		ellipse     (global.canvas, xLeft, xTop, x_radius, y_radius, sh_color);
		textout_ex(global.canvas, font, title ? title : "", txtLeft , top + 4, fg_color, -1);

		global.make_update (left, top, width, height);
		drawn = true;
	}
}
