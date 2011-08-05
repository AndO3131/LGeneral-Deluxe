/***************************************************************************
                          config.h  -  description
                             -------------------
    begin                : Tue Feb 13 2001
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

#ifndef __CONFIG_H
#define __CONFIG_H

/* stupid name similarity */
#ifdef HAVE_CONFIG_H
#  include <../config.h>
#endif

/* configure struct */
typedef struct {
    /* directory to save config and saved games */
    char dir_name[512];
    /* gfx options */
    int grid; /* hex grid */
    int tran; /* transparancy */
    int show_bar; /* show unit's life bar and icons */
    int width, height, fullscreen;
    int anim_speed; /* scale animations by this factor: 1=normal, 2=doubled, ... */
    /* game options */
    int supply; /* units must supply */
    int weather; /* does weather have influence? */
    int fog_of_war; /* guess what? */
    int show_cpu_turn;
    int deploy_turn; /* allow deployment */
    int purchase; /* disable predefined reinfs and allow purchase by prestige */
    int ai_debug; /* level of information about AI move */
    /* audio stuff */
    int sound_on;
    int sound_volume;
    int music_on;
    int music_volume;
} Config;

/* check if config directory exists; if not create it and set config_dir */
void check_config_dir_name();

/* set config to default */
void reset_config();

/* load config */
void load_config( );

/* save config */
void save_config( );

#endif
