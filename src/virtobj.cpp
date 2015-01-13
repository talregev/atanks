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
#include "environment.h"


VIRTUAL_OBJECT::VIRTUAL_OBJECT()
{
    _bitmap = NULL;
    _env = NULL;
    _global = NULL;
    x = 0;
    y = 0;
    xv = yv = 0;
    angle = 0;
    destroy = FALSE;
    age = 0;
    maxAge = -1;
    physics = 0;
    _current.x = 0;
    _current.y = 0;
    _current.w = 0;
    _current.h = 0;
    _old.x = 0;
    _old.y = 0;
    _old.w = 0;
    _old.h = 0;
}

VIRTUAL_OBJECT::~VIRTUAL_OBJECT()
{
    _bitmap = NULL;
    // player, _env and _global must be handled by descendants!
}

/** @brief update
  *
  * This method triggers an update of the canvas (aka drawing area) with the dimensions and position
  * of this object.
  */
void VIRTUAL_OBJECT::update()
{
    if (_requireUpdate)
    {
        int changed;

        if (_current.w)
        {
            if (_align == LEFT)
                _env->make_update(_current.x, _current.y, _current.w, _current.h);
            else if (_align == RIGHT)
                _env->make_update(_current.x - _current.w, _current.y - _current.h, _current.w, _current.h);
            else
                _env->make_update(_current.x - (_current.w / 2), _current.y - (_current.h / 2), _current.w + 2, _current.h + 2);
        }

        if ((_old.x == _current.x) && (_old.y == _current.y) && (_old.w == _current.w) && (_old.h == _current.h))
            changed = FALSE;
        else
            changed = TRUE;

        if (changed)
        {
            if (_old.w)
            {
                if (_align == LEFT)
                    _env->make_update(_old.x, _old.y, _old.w, _old.h);
                else if (_align == RIGHT)
                    _env->make_update(_old.x - _old.w, _old.y - _old.h, _old.w, _old.h);
                else
                    _env->make_update(_old.x - (_old.w / 2), _old.y - (_old.h / 2), _old.w + 2, _old.h + 2);
            }
            _old.x = _current.x;
            _old.y = _current.y;
            _old.w = _current.w;
            _old.h = _current.h;
        }
        _requireUpdate = FALSE;
    }
}

void VIRTUAL_OBJECT::initialise()
{
    age = 0;
    maxAge = -1;
    x = 0;
    y = 0;
    xv = 0;
    yv = 0;
    destroy = false;
    _current.x = 0;
    _current.y = 0;
    _current.w = 0;
    _current.h = 0;
    _old.x = 0;
    _old.y = 0;
    _old.w = 0;
    _old.h = 0;
}

int VIRTUAL_OBJECT::applyPhysics()
{
    x += xv;
    y += yv;

    return 0;
}

void VIRTUAL_OBJECT::draw(BITMAP *dest)
{
    if (!destroy)
    {
        rotate_sprite(dest, _bitmap, (int) x - (_bitmap->w / 2), (int) y - (_bitmap->h / 2), itofix(angle));
        if (angle == 0)
            setUpdateArea((int) x - (_bitmap->w / 2 + 1), (int) y - (_bitmap->h / 2 + 1), _bitmap->w + 2, _bitmap->h + 2);
        else
        {
            int length = MAX(_bitmap->w, _bitmap->h);
            length += MIN(_bitmap->w, _bitmap->h) / 2;
            setUpdateArea((int) x - (length/2), (int) y - (length/2), length, length);
        }
        requireUpdate();
    }
}

void VIRTUAL_OBJECT::setUpdateArea(int left, int top, int width, int height)
{
    _current.x = left;
    _current.y = top;
    _current.w = width;
    _current.h = height;
}

void VIRTUAL_OBJECT::addUpdateArea(int left, int top, int width, int height)
{
    if (left < _current.x)
        _current.x = left;
    if (top < _current.y)
        _current.y = top;
    if (left + width > _current.x + _current.w)
        _current.w = width;
    if (top + height > _current.y + _current.h)
        _current.h = height;
}
