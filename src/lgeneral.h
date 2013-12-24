/***************************************************************************
                          unit_lib.h  -  description
                             -------------------
    begin                : Sat Mar 16 2002
    copyright            : (C) 2001 by Michael Speck
    email                : kulkanie@gmx.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __LGENERAL_H
#define __LGENERAL_H

#define MAX_BUFFER 4096
#define MAX_LINE_SHORT 1024
#define MAX_PATH 512
#define MAX_LINE 256
#define MAX_NAME 50
#define MAX_EXTENSION 10

/*
====================================================================
General system includes.
====================================================================
*/
#include <SDL.h>
#ifdef WITH_SOUND
    #include <SDL_mixer.h>
    #include "audio.h"
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/*
====================================================================
Important local includes
====================================================================
*/
#include "sdl.h"
#include "image.h"
#include "misc.h"
#include "list.h"
#include "config.h"

#endif
