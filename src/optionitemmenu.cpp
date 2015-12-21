#include "optionitemmenu.h"
#include "menu.h"
#include "clock.h"

/** @brief Default constructor.
  *
  * The target is the Menu instance to handle.
  *
  * Activation is a simple call to the Menus operator(), its return value
  * is then returned without further ado.
  *
  * @param[in,out] menu_ Pointer to the Menu instance to handle.
  * @param[in] title_ The title of the option to display.
  * @param[in] titleIdx_ Index value of the submitted title. -1 means @a title_ is fixed.
  * @param[in] color Regular display color of the title.
  * @param[in] top_ Top position of the display area.
  * @param[in] left_ Left position of the display area.
  * @param[in] width_ Width of the display area.
  * @param[in] height_ Height of the display area.
  * @param[in] padding_ Padding of the title and buttons to the display area.
**/
OptionItemMenu::OptionItemMenu(
            Menu*         menu_,
            const char*   title_,
            int32_t       titleIdx_,
            int32_t       color_,
            int32_t top_, int32_t left_, int32_t width_, int32_t height_,
            int32_t padding_) :
	OptionItemBase(ET_MENU,
	               title_ ? title_ : menu_ ? menu_->getTitle() : nullptr,
	               titleIdx_,
	               nullptr, color_,
	               TC_NONE, nullptr,
	               top_, left_, width_, height_, padding_, 0),
	menu(menu_)
{
	// Both action or player must be set
	assert ( menu_ && "A nullptr menu_ makes no sense...");
	// As the title is displayed as text, textOnly must be set:
	this->textOnly = true;
}


/// @brief default dtor only setting nullptr values. No further action needed.
OptionItemMenu::~OptionItemMenu()
{
	menu = nullptr;
}


/* ----------------------
 * --- Public methods ---
 * ----------------------
 */

/** @brief activate the sub menu
  *
  * This calls operator() on the menu.
  *
  * Note: The parameters are defined by OptionItemBase but unused
  * here.
  *
  * @return The return code of the sub menu.
**/
int32_t OptionItemMenu::activate(int32_t, int32_t, int32_t, int32_t)
{
	// Remove parent menu timer
	WIN_CLOCK_REMOVE

	int32_t result = (*menu)();

	// Changes are displayed at once:
	this->drawn = false;
	this->display(false);

	// Re-add parent menu timer
	WIN_CLOCK_INIT

	return result;
}


/// @brief returns always true
bool OptionItemMenu::canGoDown()
{
	return true;
}


/// @brief returns always true
bool OptionItemMenu::canGoUp()
{
	return true;
}


/** @brief display the sub menu title
  *
  * @param[in] show_full If set to true, title and buttons are redrawn.
**/
void OptionItemMenu::display(bool show_full)
{
	this->displayMenu(menu);

	// Show decorations if wanted:
	if (show_full)
		this->displayDeco();
}


/// @brief return true, the menu must be able to return an exit code.
bool OptionItemMenu::isExitButton()
{
	return true;
}


/// @brief simply calls setLanguage(false) on the target menu
void OptionItemMenu::setLanguage()
{
	if (menu)
		menu->setLanguage(false);
}
