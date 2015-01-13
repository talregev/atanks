#ifndef BUTTON_HEADER_
#define BUTTON_HEADER_

#include "globaldata.h"
#include "environment.h"

class BUTTON
{
    GLOBALDATA *_global;
    ENVIRONMENT *_env;

public:
    BOX location;
    int xl, yl;
    char *text;
    BITMAP *bmp;
    BITMAP *hover;
    BITMAP *depressed;
    SAMPLE *click;

    BUTTON(GLOBALDATA *global, ENVIRONMENT *env, int x1, int y1, char *text1,
           BITMAP *bmp1, BITMAP *hover1, BITMAP *depressed1);

    int isPressed(); //if button pressed returns 1
    int isMouseOver(); //Cursor is over button
    void draw(BITMAP *dest);
};

#endif
