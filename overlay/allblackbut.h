/*****************************************************************************/

/*
 *      allblackbut.h -- REALmagic calibration application 
 *
 *      Copyright (C) 2000 Sigma Designs
 *                    written by Nicolas Vignal <nicolas_vignal@sdesigns.com>
 *                    modified by Emmanuel Michon <emmanuel_michon@sdesigns.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define NUMCOLORS 5

#define MY_BLACK 0
#define MY_WHITE 1
#define MY_DEEPBLUE 2
#define MY_FULLBLUE 3
#define MY_GREY 4

void AllBlackButInit();
void AllBlackButClose();
void AllBlackButPattern(int my_color,int x,int y,int width,int height);
void AllBlackButPatternRGB(int rgb_color,int x,int y,int width,int height);
void AllDrawRectangle(int fg,int bg, int x,int y,int width,int height);
