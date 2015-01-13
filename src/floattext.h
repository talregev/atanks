#ifndef FLOATTEXT_DEFINE
#define FLOATTEXT_DEFINE

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


#include "virtobj.h"
#include "main.h"
#include "environment.h"

#define newmakecol(r,g,b) makecol((int)(r),(int)(g),(int)(b))

#define NO_SWAY 0
#define NORMAL_SWAY 42

#define SPEED_RANGE 6

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

class FLOATTEXT: public VIRTUAL_OBJECT
{
private:
    // empty ctor, copy-ctor and assign operator are private, so the compiler won't create implicit ones!
    // inline const FLOATTEXT& operator= (const FLOATTEXT &sourceText) { return(sourceText); }

    char *_text;
    int _color;
    int _halfColor; // for shadowd text!
    int original_x;
    int delta_x;

public:
    int sway;

    /* --- constructor --- */
    FLOATTEXT(GLOBALDATA *global, ENVIRONMENT *env, char *text, int xpos, int ypos, int color, alignType alignment);

    /* --- destructor --- */
    ~FLOATTEXT();
    void setEnvironment(ENVIRONMENT *env);

    inline virtual void initialise()
    {
        VIRTUAL_OBJECT::initialise();
    }

    int applyPhysics();
    void draw(BITMAP *dest);
    int set_text(char * text);
    void set_pos(int x, int y);
    void set_color(int color);
    void set_speed(double x_speed, double y_speed);
    

    inline virtual int isSubClass(int classNum)
    {
        if (classNum != FLOATTEXT_CLASS)
            return FALSE;
        //return (VIRTUAL_OBJECT::isSubClass (classNum));
        else
            return TRUE;
    }

    inline virtual int getClass()
    {
        return FLOATTEXT_CLASS;
    }

    void newRound();

};

#endif
