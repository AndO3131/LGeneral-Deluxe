/***************************************************************************
                          ai.h  -  description
                             -------------------
    begin                : Thu Apr 11 2002
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

#ifndef __AI_H
#define __AI_H

/*
====================================================================
Initiate turn
====================================================================
*/
void ai_init( void );

/*
====================================================================
Queue next actions (if these actions were handled by the engine
this function is called again and again until the end_turn
action is received).
====================================================================
*/
void ai_run( void );

/*
====================================================================
Undo the steps (e.g. memory allocation) made in ai_init()
====================================================================
*/
void ai_finalize( void );

#endif
