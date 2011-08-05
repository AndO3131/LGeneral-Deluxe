/***************************************************************************
                          event.h  -  description
                             -------------------
    begin                : Wed Mar 20 2002
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
 
#ifndef __EVENT_H
#define __EVENT_H

/*
====================================================================
Event filter. As long as this is active no KEY or MOUSE events
will be available by SDL_PollEvent().
====================================================================
*/
int event_filter( const SDL_Event *event );

/*
====================================================================
Enable/Disable event filtering of mouse and keyboard.
====================================================================
*/
void event_enable_filter( void );
void event_disable_filter( void );
void event_clear( void );

/*
====================================================================
Check if an motion event occured since last call.
====================================================================
*/
int event_get_motion( int *x, int *y );
/*
====================================================================
Check if a button was pressed and return its value and the current
mouse position.
====================================================================
*/
int event_get_buttondown( int *button, int *x, int *y );
int event_get_buttonup( int *button, int *x, int *y );

/*
====================================================================
Get current cursor position.
====================================================================
*/
void event_get_cursor_pos( int *x, int *y );

/*
====================================================================
Check if 'button' button is currently set.
====================================================================
*/
enum {
    BUTTON_NONE = 0,
    BUTTON_LEFT,
    BUTTON_MIDDLE,
    BUTTON_RIGHT,
    WHEEL_UP,
    WHEEL_DOWN
};
int event_check_button( int button );

/*
====================================================================
Check if 'key' is currently set.
====================================================================
*/
int event_check_key( int key );

/*
====================================================================
Check if SDL_QUIT was received.
====================================================================
*/
int event_check_quit();

/*
====================================================================
Clear the SDL event key (keydown events)
====================================================================
*/
void event_clear_sdl_queue();

/*
====================================================================
Wait until neither a key nor a button is pressed.
====================================================================
*/
void event_wait_until_no_input();

#endif
