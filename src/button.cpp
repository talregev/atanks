/*
This file contains funcions for the BUTTON class. These
are being moved out of the atanks.cc file.
-- Jesse
*/

#include "button.h"

BUTTON::BUTTON(GLOBALDATA *global, ENVIRONMENT *env, int x1, int y1,
               char *text1, BITMAP *bmp1, BITMAP *hover1, BITMAP *depressed1):_global(NULL),_env(NULL),text(NULL),
               bmp(NULL),hover(NULL),depressed(NULL),click(NULL)
{
    _global = global;
    _env = env;
    location.x = x1;
    location.y = y1;
    text = text1;
    click = (SAMPLE *)_global->sounds[8];
    bmp = bmp1;
    hover = hover1;
    depressed = depressed1;
    location.w = bmp->w;
    location.h = bmp->h;
    xl = location.x + bmp->w;
    yl = location.y + bmp->h;
}



void BUTTON::draw(BITMAP *dest)
{
    draw_sprite(dest, (BITMAP *)(isMouseOver()?(isPressed()?depressed:hover):bmp), location.x, location.y);
    if (text)
        textout_centre_ex (dest, font, text, location.x+75, location.y + 6, WHITE, -1);
    _env->make_update(location.x, location.y, xl, yl);
}


int BUTTON::isMouseOver()
{
    if ((mouse_x >= location.x) && (mouse_y >= location.y) && (mouse_x <= xl) && (mouse_y <= yl))
        return 1;
    else
        return 0;
}


int BUTTON::isPressed()
{
    if ((mouse_b == 1) && isMouseOver())
    {
        play_sample (click, _env->scaleVolume(128), 128, 1000, 0);
        return 1;
    }
    else
        return 0;
}
