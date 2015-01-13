#ifndef EXTERNS_DEFINE
#define EXTERNS_DEFINE

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


extern char *errorMessage;
extern int errorX, errorY;

extern int WHITE, BLACK, PINK, COLOR[];

extern int k;
extern int ctrlUsedUp;

extern int cclock, fps, frames, fi, lx, ly, order[];

//extern char cacheCirclesBG;
//extern char ditherGradients;
//extern int startmoney;
//extern int turntype;

extern double height[];
//extern int landtable[];

//extern int steep, mheight, mbase;
//extern double msteep, smooth;
//extern double gravity;
//extern double windstrength, windvariation;
//extern double interest;
//extern char name[][11];

extern int winner;

extern WEAPON weapon[];
extern WEAPON naturals[];
extern ITEM item[];

#endif
