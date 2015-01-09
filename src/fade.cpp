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

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
fade.cc

Provides graphical transitions via change and quickChange, as prototyped in
main.h
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

/*
TODO
 + Add more faders
 + Improve/complete documentation
*/



#include "globaldata.h"
#include "main.h"



/*****************************************************************************
Internal fades

The following section of code defines the internal fade routines.  These
routines are used by the change function to perform transitions.  To add a
new fader, simply implement a function as seen below and then add it to
change's function pointer array.
*****************************************************************************/
/*
static void spiralPixelFade (GLOBALDATA *global, BITMAP *target)
{
  int blockSize = 8;
  int spiralSize = 20;
  int length = spiralSize, x = 0, y = 0;
  int stepSize = (spiralSize + 1) * blockSize;
  int x2, y2;
  int count;

  for (count = 0, y = stepSize - blockSize; count < length; count++, y -= blockSize)
    for (x2 = 0; x2 <= global->screenWidth; x2 += stepSize)
      for (y2 = 0; y2 <= global->screenHeight; y2 += stepSize)
        blit (target, screen, x2+x, y2+y, x2+x, y2+y, blockSize, blockSize);
  while (length)
    {
      for (count = 0; count < length; count++, x += blockSize)
        for (x2 = 0; x2 <= global->screenWidth; x2 += stepSize)
          for (y2 = 0; y2 <= global->screenHeight; y2 += stepSize)
            blit (target, screen, x2+x, y2+y, x2+x, y2+y, blockSize, blockSize);
      for (count = 0; count < length; count++, y += blockSize)
        for (x2 = 0; x2 <= global->screenWidth; x2 += stepSize)
          for (y2 = 0; y2 <= global->screenHeight; y2 += stepSize)
            blit (target, screen, x2+x, y2+y, x2+x, y2+y, blockSize, blockSize);
      length--;
      for (count = 0; count < length; count++, x -= blockSize)
        for (x2 = 0; x2 <= global->screenWidth; x2 += stepSize)
          for (y2 = 0; y2 <= global->screenHeight; y2 += stepSize)
            blit (target, screen, x2+x, y2+y, x2+x, y2+y, blockSize, blockSize);
      for (count = 0; count < length; count++, y -= blockSize)
        for (x2 = 0; x2 <= global->screenWidth; x2 += stepSize)
          for (y2 = 0; y2 <= global->screenHeight; y2 += stepSize)
            blit (target, screen, x2+x, y2+y, x2+x, y2+y, blockSize, blockSize);
      length--;
    }
  for (x2 = 0; x2 <= global->screenWidth; x2 += stepSize)
    for (y2 = 0; y2 <= global->screenHeight; y2 += stepSize)
      blit (target, screen, x2+x, y2+y, x2+x, y2+y, blockSize, blockSize);
}

static void topDownMultiWipeFade (GLOBALDATA *global, BITMAP *target)
{
  int z, zz ;

  for (zz = 0; zz < 8; zz++)
    {
      int lineCount = 0;
      for (z = zz; z < global->screenHeight; z += 8)
        {
          blit (target, screen, 0, z, 0, z, global->screenWidth, 1);
          if (lineCount == 10)
            {
              lineCount = 0;
              rest (1);
            }
          lineCount++;
        }
    }
}

static void topDownWipeFade (GLOBALDATA *global, BITMAP *target)
{
  int z;

  for (z = 0; z <= global->screenHeight / 8; z++)
    {
      blit (target, screen, 0, z * 8, 0, z * 8, global->screenWidth, 8);
      rest (1);
    }
}

static void bottomUpWipeFade (GLOBALDATA *global, BITMAP *target)
{
  int z;

  for (z = global->screenHeight / 8; z >= 0; z--)
    {
      blit (target, screen, 0, z * 8, 0, z * 8, global->screenWidth, 8);
      rest (1);
    }
}

static void rightLeftWipeFade (GLOBALDATA *global, BITMAP *target)
{
  int z;

  for (z = global->screenWidth / 8; z >= 0; z--)
    {
      blit (target, screen, z * 8, 0, z * 8, 0, 8, global->screenHeight);
      rest (1);
    }
}

static void leftRightWipeFade (GLOBALDATA *global, BITMAP *target)
{
  int z;

  for (z = 0; z <= global->screenWidth / 8; z++)
    {
      blit (target, screen, z * 8, 0, z * 8, 0, 8, global->screenHeight);
      rest (1);
    }
}

static void crossLinesFade (GLOBALDATA *global, BITMAP *target)
{
  int z;

  for (z = 0; z < (global->screenWidth/4+4) / 4; z++)
    {
      blit (target, screen, z * 8, 0, z * 8, 0, 4, global->screenHeight);
      blit (target, screen, (global->screenWidth-1) - (z * 8), 0, (global->screenWidth-1) - (z * 8), 0, 4, global->screenHeight);
      blit (target, screen, 0, (z * 6), 0, (z * 6), global->screenWidth, 4);
      blit (target, screen, 0, (global->screenHeight - (z * 6)), 0, (global->screenHeight - (z * 6)), global->screenWidth, 4);
      rest (1);
    }
  for (z = (global->screenWidth/4+4) / 4; z != 0; z--)
    {
      blit (target, screen, z * 8 + 4, 0, z * 8 + 4, 0, 4, global->screenHeight);
      blit (target, screen, (global->screenWidth-1) - (z * 8) + 4, 0, (global->screenWidth-1) - (z * 8) + 4, 0, 4, global->screenHeight);
      blit (target, screen, 0, (z * 6) + 4, 0, (z * 6) + 4, global->screenWidth, 4);
      blit (target, screen, 0, (global->screenHeight - (z * 6)) + 4, 0, (global->screenHeight - (z * 6)) + 4, global->screenWidth, 4);
      rest (1);
    }
}

*/

/*****************************************************************************
changeHandleError

Used internally by change and quickChange.  Prints out error messages.
*****************************************************************************/
static void changeHandleError (GLOBALDATA *global, BITMAP *target)
{
  if (! global->os_mouse) show_mouse (NULL);
  if (errorMessage)
    {
      textout_ex (target, font, errorMessage, errorX, errorY, makecol (255, 0, 0), -1);
      errorMessage = NULL;
    }
}


/*****************************************************************************
change

Selects and executes a random transition.
*****************************************************************************/
/*
void change(GLOBALDATA *global, BITMAP *target)
{
  static void (* const procs[])(GLOBALDATA*,BITMAP*) =
  {
    crossLinesFade,			spiralPixelFade,
    leftRightWipeFade,		rightLeftWipeFade,
    bottomUpWipeFade,		topDownWipeFade,
    topDownMultiWipeFade,
  } ;
  static const int cprocs = sizeof( procs ) / sizeof( procs[0] );

  if (! global->os_mouse) show_mouse (NULL);
  changeHandleError (global, target);

  (procs[rand() % cprocs]) (global, target);

  clear_keybuf ();
}
*/

void change(GLOBALDATA *global, BITMAP *target)
{
    quickChange(global, target);
}

/*****************************************************************************
quickChange

Executes a fast and simple transition.

*****************************************************************************/
void quickChange (GLOBALDATA *global, BITMAP *target)
{
  changeHandleError (global, target);
  blit (target, screen, 0, 0, 0, 0, global->screenWidth, global->screenHeight);
  rest (1);
}

