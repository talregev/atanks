#ifndef	MENU_HEADER
#define	MENU_HEADER

/*
 * atanks - obliterate each other with oversize weapons
 * Copyright (C) 2003  Thomas Hudson
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
 * */
#include "globaldata.h"


#define PLAY_GAME 1
#define LOAD_GAME 2
#define ESC_MENU 3

#define TEXT_BOX_LENGTH 14


enum	menuEntryType
{
  OPTION_MENUTYPE, OPTION_DOUBLETYPE, OPTION_TOGGLETYPE, OPTION_SPECIALTYPE, OPTION_ACTIONTYPE, OPTION_TEXTTYPE, OPTION_COLORTYPE
};
typedef struct
  {
    const char	*name;
    int	(*displayFunc) (ENVIRONMENT*, int, int, void*);
    int	color;
    double	*value;
    void	*data;
    const char	*format;
    double	min, max;
    double	increment;
    double	defaultv;
    char	**specialOpts;
    char	type;
    int	viewonly;
    int	x;
    int	y;
  } MENUENTRY;

typedef struct
  {
    const char *title;
    int numEntries;
    MENUENTRY *entries;
    int quitButton;
    int okayButton;
  } MENUDESC;


// Set the menus to appear in English or Portugese
void Select_Menu_Language(GLOBALDATA *global);

int options (GLOBALDATA *global, ENVIRONMENT *env, MENUDESC *menu);

#endif