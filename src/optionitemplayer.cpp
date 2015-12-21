#include "optionitemplayer.h"
#include "player.h"
#include "floattext.h"

/** @brief Default constructor.
  *
  * The target is the player instance to handle. And the @a action_ function
  * must do this if set.
  *
  * The target is a pointer pointer, so the class can be used for both
  * editing and creating a player.
  *
  * If @a title_ is nullptr, the player name and color are used. The set
  * title and BLACK otherwise.
  *
  * This class can be both, an ET_MENU (If an @a action_ function is set)
  * or an ET_TOGGLE otherwise.
  *
  * @param[in,out] player_ Pointer to the PLAYER instance to handle.
  * @param[in,out] action_ Pointer to the action function handling the button click.
  * @param[in] title_ The title of the option to display.
  * @param[in] titleIdx_ Index value of the submitted title. -1 means @a title_ is fixed.
  * @param[in] top_ Top position of the display area.
  * @param[in] left_ Left position of the display area.
  * @param[in] width_ Width of the display area.
  * @param[in] height_ Height of the display area.
  * @param[in] padding_ Padding of the title and buttons to the display area.
**/
OptionItemPlayer::OptionItemPlayer(
            PLAYER** player_,
            int32_t (*action_)(PLAYER** player_, int32_t),
            const char*  title_,
            int32_t      titleIdx_,
            int32_t top_, int32_t left_, int32_t width_, int32_t height_,
            int32_t padding_) :
	OptionItemBase(ET_NONE,
	               title_ ? title_
	               : (player_ && *player_)
	                 ? (*player_)->getName()
	                 : nullptr,
	               titleIdx_,
	               nullptr,
	               title_ ? SILVER
	               : (player_ && *player_)
	                 ? (*player_)->color
	               : WHITE, // WHITE to indicate an error.
	               TC_NONE, nullptr,
	               top_, left_, width_, height_, padding_, 0)
{
	// For ET_TOGGLE, only the player_ is needed
	assert (player_ && "ERROR: player_ must be set");

	// For ET_MENU, action_ must be set, too, and for ET_TOGGLE *player_ must be set.
	assert ( (action_ || (player_ && *player_ ) )
		  && "ERROR: If no action_ function is set, *player_ must be valid");
	actionFunc = action_;
	player     = player_;

	if (actionFunc) {
		this->type = ET_MENU;

		// If this is a regular player, no menu indicator is needed.
		if (nullptr == title_)
			this->show_menu = false;
	} else if (player && *player)
		this->type = ET_TOGGLE;
}


/// @brief default dtor only setting nullptr values. No further action needed.
OptionItemPlayer::~OptionItemPlayer()
{
	actionFunc  = nullptr;
	player      = nullptr;
}


/* ----------------------
 * --- Public methods ---
 * ----------------------
 */

/** @brief activate the player sub menu
  *
  * This calls the provided action function.
  *
  * Note: The parameters are defined by OptionItemBase but unused
  * here.
  *
  * @return The return code of the action function.
**/
int32_t OptionItemPlayer::activate(int32_t, int32_t, int32_t, int32_t)
{
	int32_t result = -1;

	if (ET_MENU == this->type)
		result = actionFunc(player, 0);
	else if (ET_TOGGLE == this->type)
		this->activateToggle(&(*player)->selected);

	// Changes are displayed at once:
	if (ET_NONE != this->type)
		this->clear_display(true);

	return result;
}


/// @brief returns always true
bool OptionItemPlayer::canGoDown()
{
	return true;
}


/// @brief returns always true
bool OptionItemPlayer::canGoUp()
{
	return true;
}


/** @brief display the sub menu title
  *
  * @param[in] show_full If set to true, title and buttons are redrawn.
**/
void OptionItemPlayer::display(bool show_full)
{
	static const int32_t team_col_hi       = 0xc0;
	static const int32_t team_col_mi       = 0x40;
	static const int32_t team_col_lo       = 0x18;
	static const char*   team_Indicator[4] = { "S", "N", "J", "?" };
	static const int32_t team_color_bg[4]  = {
		makecol(team_col_mi, team_col_lo, team_col_lo),
		makecol(team_col_lo, team_col_mi, team_col_lo),
		makecol(team_col_lo, team_col_lo, team_col_mi),
		makecol(team_col_mi, team_col_mi, team_col_mi)
	};
	static const int32_t team_color_fg[4]  = {
		makecol(team_col_hi, team_col_mi, team_col_mi),
		makecol(team_col_mi, team_col_hi, team_col_mi),
		makecol(team_col_mi, team_col_mi, team_col_hi),
		makecol(team_col_hi, team_col_hi, team_col_hi)
	};

	if (!drawn) {
		// Be sure to have the current name and color:
		color = player && *player ? (*player)->color : color;
		if ( player && *player && (!title || (strcmp((*player)->getName(), title))) )
			setTitle((*player)->getName());

		// Now display the player
		int32_t tWidth   = -1 == titleIdx ? text_length(font, "W") + padding + 4 : 0;
		int32_t xOff     = -1 == titleIdx ? 15 + padding + tWidth : 0;
		int32_t txtLeft  = left + xOff;
		int32_t txtColor = color;
		int32_t xTop     = top + 1;
		int32_t xHeight  = height - 2;

		// If this is a toggle, it must be displayed first:
		if (ET_TOGGLE == this->type) {
			int32_t bgColor = BLACK;
			int32_t shColor = makecol(getr(color) / 3, getg(color) / 3, getb(color) / 3);

			// Swap colors if the player is selected
			if ((*player)->selected) {
				bgColor  = color;
				txtColor = BLACK;
			}

			// Add a button like area for the name
			rect(    global.canvas, txtLeft,     top,     left + width,     top + height,     txtColor);
			rect(    global.canvas, txtLeft + 1, top + 1, left + width - 1, top + height - 1, txtColor);
			hline(   global.canvas, txtLeft + 1,      top + height - 1, left + width - 1, shColor);
			hline(   global.canvas, txtLeft,          top + height,     left + width,     shColor);
			vline(   global.canvas, left + width - 1, top + 1,          top + height - 1, shColor);
			vline(   global.canvas, left + width,     top,              top + height,     shColor);
			rectfill(global.canvas, txtLeft + 2, top + 2, left + width - 2, top + height - 2, bgColor);

			// Additional text offset for the border
			txtLeft += 3;
			xHeight -= 2;
		}

		// Then display the player name
		eTeamTypes pTeam = player && *player ? (*player)->team : TEAM_COUNT;
		if (title && title[0]) {

			// Is the text shadowed, then create one:
			if (env.shadowedText)
				textout_ex (global.canvas, font, title, txtLeft + 2, xTop + 2,
				            GetShadeColor(txtColor, true, PINK), -1);

			textout_ex (global.canvas, font, title, txtLeft + 1, xTop + 1, txtColor, -1);
		}

		// Second the player type indicator:
		if (-1 == titleIdx ) {
			(*player)->drawIndicator(left, xTop, xHeight);

			// and third the team indicator:
			int32_t xLeft = left + 19;

			rectfill(global.canvas, xLeft + 1, xTop + 3,
			         xLeft + 12, xTop + xHeight - 2,
			         team_color_bg[pTeam]);
			rect(global.canvas, xLeft, xTop + 2, xLeft + 13, xTop + xHeight - 1,
			     team_color_fg[pTeam]);

			xLeft += 7;

			textout_centre_ex(global.canvas, font, team_Indicator[pTeam],
			                  xLeft - (TEAM_SITH == pTeam ? 1 : 0), xTop + 1,
			                  team_color_fg[pTeam], -1);
		} // end of player indicator

		global.make_update (left, top, width, height);
		drawn = true;
	}

	// Show decorations if wanted:
	if (show_full)
		this->displayDeco();
}


/// @brief return true, the action function must be able to return an exit code.
bool OptionItemPlayer::isExitButton()
{
	return true;
}
