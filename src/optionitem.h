#pragma once
#ifndef ATANKS_SRC_OPTIONITEM_H_INCLUDED
#define ATANKS_SRC_OPTIONITEM_H_INCLUDED

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

/** @file optionitem.h
  * @brief declaration of the option entry template
**/


/** @class OptionItem
  * @brief abstract one option menu entry
  *
  * One option menu entry is what the user interacts with. Each option menu
  * entry can handle exactly one target value, or none if none is needed.
  * Target values can be manipulated directly or with the help of an action
  * function set via function pointer upon creation.
  * Either a target value or an action function must be set unless the entry
  * is a sub menu.
  * In the latter case activating the option displays the sub menu and hands
  * over control to its menu handler.
  *
  * The setup is fairly easy, just use the appropriate setData() method. The
  * template itself has only two other methods: activate() and display(). See
  * OptionItemBase for more public methods.
  *
  * When using the empty ctor, remember to call it with an explicit type.
  * To be usable with the new-operator this class has an empty ctor which
  * leaves all members with neutral default values. To become a usable instance
  * the method setData() must be called with the desired values.
  *
  *
  * One <B>BIG</B> fat warning, though: Do not use ET_TEXT with anything other
  * than a char* target, and only allow the usage of OptionItem::activate()
  * if it is not full, yet!\n
  * Background: Allowing other types would make the internals insanely
  * complicated, and all string options that can be manipulated with the option
  * menu are char* strings (or arrays) already. Do not change this.
  *
  * Why a template?\n
  * With a template the option menu becomes type agnostic. This allowed to
  * correct all global, environment and player data values with inappropriate
  * types to proper types without the need to adapt the option menu.
  *
  * Template parameters:\n
  * While tgt_T defines the target type, opt_T is used for the manipulating
  * values like minimum, maximum, increment/decrement value and action function
  * typing. This is done to allow a more intuitive usage.\n
  * As an example, if the target is a double, minimum, maximum and increment
  * values can be set to -1.0, 1.0 and 0.1 without the need for a postfix 'L'.
**/
template<typename tgt_T, typename opt_T = int32_t>
class OptionItem : public OptionItemBase
{
public:

	/* -------------------------------------------
	 * --- Public constructors and destructors ---
	 * -------------------------------------------
	 */

	/** @brief default ctor, no special functions or variants.
	  *
	  * This simplest ctor defines an option that can only be ET_TEXT.
	  *
	  * @param[in,out] target_ Pointer to the value to handle.
	  * @param[in] max_ Maximum length for ET_TEXT options.
	  * @param[in] color_ Color of the text to display.
	  * @param[in] type_ Type of the option.
	  * @param[in] title_ The title of the option to display.
	  * @param[in] titleIdx_ Index value of the submitted title. -1 means @a title_ is fixed.
	  * @param[in] format_ Format string to use when displaying the ET_TEXT target.
	  * @param[in] top_ Top position of the display area.
	  * @param[in] left_ Left position of the display area.
	  * @param[in] width_ Width of the display area.
	  * @param[in] height_ Height of the display area.
	  * @param[in] padding_ Padding of the title and buttons to the display area.
	**/
	OptionItem(tgt_T*        target_,
	           opt_T         max_,
	           int32_t       color_,
	           eEntryType    type_,
	           const char*   title_,
	           int32_t       titleIdx_,
	           const char*   format_,
	           int32_t top_, int32_t left_, int32_t width_, int32_t height_,
	           int32_t padding_) :
		OptionItemBase(type_, title_, titleIdx_,
					nullptr, color_, TC_NONE, format_,
					top_, left_, width_, height_, padding_, 0),
		maxVal(max_),
		target(target_)
	{
		// The value of max_ determines whether this
		// is read only or not. If it is set, it is writable.
		// The default is true, so only if it maxVal is 0, something has to be done.
		if (maxVal)
			read_only = false;
	}


	/** @brief ctor for ET_VALUE with optional display function.
	  *
	  * This ctor creates an ET_VALUE, a different type can not be set.
	  *
	  * @param[in,out] target_ Pointer to the value to handle.
	  * @param[in] title_ The title of the option to display.
	  * @param[in] titleIdx_ Index value of the submitted title. -1 means @a title_ is fixed.
	  * @param[in] text_ Array of texts to display.
	  * @param[in] color_ Color of the text to display.
	  * @param[in] class_ The text class of @a text_.
	  * @param[in] min_ Minimum value target can become.
	  * @param[in] max_ Maximum value target can become.
	  * @param[in] decinc_ Value target is changed on each action.
	  * @param[in] format_ Format string to use when displaying the target.
	  * @param[in] top_ Top position of the display area.
	  * @param[in] left_ Left position of the display area.
	  * @param[in] width_ Width of the display area.
	  * @param[in] height_ Height of the display area.
	  * @param[in] padding_ Padding of the title and buttons to the display area.
	  * @param[in] display_ optional display function to use.
	**/
	explicit
	OptionItem(tgt_T*        target_,
				const char*  title_,
				int32_t      titleIdx_,
				const char** text_,
				int32_t      color_,
				eTextClass   class_,
				opt_T min_,  opt_T max_, opt_T decinc_,
				const char*  format_,
				int32_t top_, int32_t left_, int32_t width_, int32_t height_, int32_t padding_,
				bool (*display_)(tgt_T* target, int32_t x, int32_t y)) :
		OptionItemBase(ET_VALUE, title_, titleIdx_,
						text_, color_, class_, format_,
						top_, left_, width_, height_, padding_, 0),
		displayFunc(display_),
		decinc(decinc_),
		maxVal(max_),
		minVal(min_),
		target(target_)
	{
		// The target must be set, this variant does not allow
		// an action function:
		assert(target && "A target must be set with ET_VALUE");

		// maxVal must be larger than minVal, otherwise they are swapped
		if (maxVal < minVal) {
			opt_T tmp = minVal;
			minVal = maxVal;
			maxVal = tmp;
		}

		// If this is a text rotator, entryNum must be set to *target:
		entryNum = static_cast<int32_t>(*target);

		// Either format, texts or display must be set
		assert( (format || texts || displayFunc || (TC_NONE == class_))
			&& "Either format, texts or display must be set with ET_VALUE");
	}


	/** @brief free ctor with action function.
	  *
	  * Here the handling is done by an action function. A display
	  * function can be optionally set, too.
	  *
	  * If this is an ET_ACTION type, a display function must be set.
	  * For ET_BUTTON, an action function is mandatory.
	  *
	  * @param[in,out] target_ Pointer to the value to handle.
	  * @param[in,out] action_ Pointer to the action function handling the target.
	  * @param[in] type_ Type of the option.
	  * @param[in] title_ The title of the option to display.
	  * @param[in] titleIdx_ Index value of the submitted title. -1 means @a title_ is fixed.
	  * @param[in] text_ Array of texts to display. Only needed with ET_VALUE.
	  * @param[in] color_ Color of the text to display.
	  * @param[in] class_ The text class of @a text.
	  * @param[in] format_ Format string to use when displaying the target.
	  * @param[in] top_ Top position of the display area.
	  * @param[in] left_ Left position of the display area.
	  * @param[in] width_ Width of the display area.
	  * @param[in] height_ Height of the display area.
	  * @param[in] padding_ Padding of the title and buttons to the display area.
	  * @param[in] display_ optional display function to use.
	**/
	OptionItem(tgt_T*        target_,
	           int32_t     (*action_)(tgt_T* target, int32_t val),
	           eEntryType    type_,
	           const char*   title_,
	           int32_t       titleIdx_,
	           const char**  text_,
	           int32_t       color_,
	           eTextClass    class_,
	           opt_T min_, opt_T max_, opt_T decinc_,
	           const char*   format_,
	           int32_t top_, int32_t left_, int32_t width_, int32_t height_,
	           int32_t padding_,
	           bool        (*display_)(tgt_T* target, int32_t x, int32_t y)) :
		OptionItemBase(type_, title_, titleIdx_,
						text_, color_, class_, format_,
						top_, left_, width_, height_, padding_, 0),
		actionFunc(action_),
		displayFunc(display_),
		decinc(decinc_),
		maxVal(max_),
		minVal(min_),
		target(target_)
	{
		// Either action or target must be set
		assert ( (actionFunc || target)
			&& "Either action or target must be set");

		// If this is an ET_ACTION, both action and display
		// functions must be set:
		assert ( ( (ET_ACTION != type) || (actionFunc && displayFunc) )
			  && "ET_ACTION needs both display and action function!" );

		// An ET_MENU must have a display function set. To be more concrete,
		// it must be OptionMenu->displaySub(). (Although this isn't checked.)
		assert ( ( (ET_MENU != type) || displayFunc )
			  && "ET_MENU must have a display function (OptionMenu->displaySub()) set!");


		// If this is ET_VALUE and no action function is set, the same
		// limitations as with the ET_VALUE ctor are present.
		if (ET_VALUE == type) {
			if (maxVal < minVal) {
				opt_T tmp = minVal;
				minVal = maxVal;
				maxVal = tmp;
			}

			// If this is a text rotator, entryNum must be set to *target:
			entryNum = static_cast<int32_t>(*target);

			// Either format, texts or display must be set
			assert( (format || texts || displayFunc || (TC_NONE == class_))
				&& "Either format, texts or display must be set with ET_VALUE");
		}
	}


	/** @brief special ctor to define a button.
	  *
	  * Here the handling can be done by an action function, otherwise a key
	  * code associated with the button is returned on activation.
	  *
	  * Buttons have no target, so set a dummy type when calling the ctor.
	  *
	  * @param[in] keyCode_ The key code to return when no action function is set.
	  * @param[in,out] target_ Pointer to the value to handle.
	  * @param[in,out] action_ Pointer to the action function handling the button click.
	  * @param[in] title_ The title of the option to display.
	  * @param[in] titleIdx_ Index value of the submitted title. -1 means @a title_ is fixed.
	  * @param[in] button_ Pointer to the button to use, it must be pre-created.
	  * @param[in] top_ Top position of the display area.
	  * @param[in] left_ Left position of the display area.
	  * @param[in] width_ Width of the display area.
	  * @param[in] height_ Height of the display area.
	  * @param[in] padding_ Padding of the title and buttons to the display area.
	**/
	OptionItem(int32_t      keyCode_,
	           tgt_T*       target_,
	           int32_t    (*action_)(tgt_T* target, int32_t val),
	           const char*  title_,
	           int32_t      titleIdx_,
	           BUTTON*      button_,
	           int32_t top_, int32_t left_, int32_t width_, int32_t height_,
	           int32_t padding_) :
		OptionItemBase(ET_BUTTON, title_, titleIdx_,
						nullptr, BLACK, TC_NONE, nullptr,
						top_, left_, width_, height_, padding_, 0),
		actionFunc(action_),
		target(target_)
	{
		// Either action or keyCode must be set
		assert ( (actionFunc || keyCode_)
			&& "Either action_ or keyCode_ must be set");

		if (keyCode_) this->keyCode = keyCode_;
		if (button_) {
			this->button = button_;
			this->button->getLocation(this->left, this->top, this->width, this->height);
		}
	}


	/// @brief default dtor only setting nullptr values. No further action needed.
	~OptionItem()
	{
		actionFunc  = nullptr;
		displayFunc = nullptr;
		target      = nullptr;
	}


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	/** @brief activate the option handling
	  *
	  * This activates whatever the option is configured for.
	  *
	  * If an action function is set, it is simply called. The parameter is
	  * then ignored. If this is an ET_VALUE and no action function is set,
	  * the configured increment/decrement value is applied according to @a val.
	  * If this is an ET_TEXT option, the appropriate ways to get the target
	  * value are used.
	  *
	  * @param[in] val Used for ET_VALUE: <0 = decrement, >0 = increment.
	  * @param[in] ignored (see OptionItemColour)
	  * @param[in] ignored (see OptionItemColour)
	  * @param[in] k The latest key press to use on an ET_TEXT.
	  * @return normally 0, but ET_BUTTON and ET_MENU can return key_codes
	  * assigned with exit buttons.
	**/
	int32_t activate(int32_t val, int32_t, int32_t, int32_t k)
	{
		int32_t result = 0;

		if (actionFunc) {
			result = actionFunc(target, val);

			// Here it is important that the action function does the right
			// thing with the target if this is an ET_VALUE and val<>0
			if ((ET_VALUE == type) && val && texts)
				entryNum = static_cast<int32_t>(*target);

		} else {
			// Here a target must be set as there is no action function.
			if ((ET_BUTTON != type) && !target && !actionFunc) {
				cerr << "\n" << __FUNCTION__ << ": Illegal setup!" << endl;
				cerr << "A target value MUST be set with ET_";
				cerr << ( ET_COLOR  == type ? "COLOR"
						: ET_TEXT   == type ? "TEXT"
						: ET_TOGGLE == type ? "TOGGLE"
						: ET_VALUE  == type ? "VALUE" : "UNKNOWN") << endl;
				return false;
			}

			// Every type has its own base class activation function,
			// so a simple switch will do.
			switch(type) {
				case ET_BUTTON:
					// Can trigger end events
					result = this->keyCode;
					break;
				case ET_MENU:
					result = this->activateMenu(target);
					break;
				case ET_TEXT:
					if (!read_only)
						this->activateText(target, k);
					break;
				case ET_TOGGLE:
					this->activateToggle(target);
					break;
				case ET_VALUE:
					this->activateValue(val);
					break;
				case ET_COLOR:
				case ET_NONE:
				case ET_ACTION:
				default:
					cerr << "\n" << __FUNCTION__ << ": Illegal setup!" << endl;
					cerr << " No action function set when it is needed!" << endl;
					return false;
			} // End of switch(type)
		} // End of having no action function

		// Changes are displayed at once:
		// (unless this is ET_COLOR, it has been drawn already.)
		if (ET_COLOR != this->type) {
			this->clear_display(true);
			this->display(true);
		}

		return result;
	}


	/// @brief return true if the target has not reached its minimum, yet
	virtual bool canGoDown()
	{
		if ( (ET_VALUE == this->type) && this->format)
			// Check format, because texts[] based options are rotated.
			return (*target > static_cast<tgt_T>(minVal) ? true : false);
		return true;
	}


	/// @brief return true if the target has not reached its maximum, yet
	virtual bool canGoUp()
	{
		if ( (ET_VALUE == this->type) && this->format)
			return (*target < static_cast<tgt_T>(maxVal) ? true : false);
		return true;
	}


	/** @brief display the option content
	  *
	  * This method either calls the set display function or its own one.\n
	  * ET_ACTION and ET_MENU <B>must</B> have set a display function.
	  *
	  * If @a show_full is set to true, the option title and possible
	  * button(s) are displayed, too. This is useful to initially display a
	  * menu or when switching languages.
	  *
	  * However, if the option is either ET_MENU or ET_ACTION, @a show_full
	  * is ignored.
	  *
	  * @param[in] show_full If set to true, title and buttons are redrawn.
	**/
	void display(bool show_full)
	{
		if (displayFunc) {
			clear_display(false);
			displayFunc(target, left, top);
			drawn = true;
		} else {
			// Every type has its own base class display function,
			// so a simple switch will do.
			switch(type) {
				case ET_BUTTON:
					this->displayButton();
					break;
				case ET_MENU:
					this->displayMenu(target);
					break;
				case ET_TEXT:
					this->displayText(target);
					break;
				case ET_TOGGLE:
					this->displayToggle(target);
					break;
				case ET_VALUE:
					this->displayValue(target);
					break;
				case ET_NONE:
				case ET_ACTION:
				case ET_COLOR:
				default:
					cerr << "\n" << __FUNCTION__ << ": Illegal setup!" << endl;
					cerr << " No display function set when it is needed!" << endl;
					return;
			} // End of switch(type)
		} // end of having no display function

		// Show decorations if wanted: (ET_COLOR does that in displayColor())
		if (show_full && (ET_COLOR != type))
			this->displayDeco();
	}


	/// @brief return true if this is an ET_BUTTON with a key code and no action function.
	bool isExitButton()
	{
		return ((ET_BUTTON == type) && (nullptr == actionFunc) && (-1 < keyCode));
	}


	/// @brief Quickly change (or set) the action function
	void setAction(int32_t (*action_)(tgt_T* target, int32_t val))
	{
		actionFunc = action_;
	}


private:

	/* ----------------------------------------------
	 * --- Private methods and external functions ---
	 * ----------------------------------------------
	 */

	int32_t (*actionFunc )(tgt_T* target, int32_t val)          = nullptr;
	bool    (*displayFunc)(tgt_T* target, int32_t x, int32_t y) = nullptr;

	/// @brief templated ET_VALUE activation handling
	void activateValue(int32_t val)
	{
		// A few short-cuts that make reading the following a lot easier:
		tgt_T t_val = static_cast<tgt_T>(val * decinc);
		tgt_T t_max = static_cast<tgt_T>(maxVal);
		tgt_T t_min = static_cast<tgt_T>(minVal);

		if (format) {
			// If a format is set, this is just a simple adding/substracting
			// of decinc with a check against min/max value afterwards
			// val == 0 is simply ignored.
			tgt_T oldTgt = *target;
			if (val > 0) {
				if (*target <= (t_max - t_val) )
					*target += t_val;
				else
					*target = t_max;
			} else if (val < 0) {
				if (*target >= (t_min - t_val) )
					*target += t_val;
				else
					*target = t_min;
			}
			// If a maximum or minimum is reached, clear the decoration
			if ( (oldTgt != *target)
			  && ( (*target == t_min)
				|| (*target == t_max) ) ) {
				clear_display(true);
				display(true);
			}
		} else if (texts) {
			// Otherwise entryNum is used and checked against texts[]
			if (val > 0) {
				if(  texts[entryNum + 1]
				  && (*target < t_max ) )
					++entryNum;
				else
					entryNum = 0;
			} else if (val < 0) {
				if(  entryNum > 0
				  && (*target > static_cast<tgt_T>(0) ) )
					--entryNum;
				else
					entryNum = t_max;
			}
			*target = static_cast<tgt_T>(entryNum);
		}
	}


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	opt_T  decinc  = (opt_T)1; //!< Increment / decrement for ET_VALUE
	opt_T  maxVal  = (opt_T)0; //!< Maximum value for ET_VALUE
	opt_T  minVal  = (opt_T)0; //!< Minimum value for ET_VALUE
	tgt_T* target  = nullptr;  //!< Target to handle
};


#endif // ATANKS_SRC_OPTIONITEM_H_INCLUDED

