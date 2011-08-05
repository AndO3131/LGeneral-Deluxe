/***************************************************************************
                          audio.c  -  description
                             -------------------
    begin                : Sun Jul 29 2001
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

#ifdef WITH_SOUND

#include <SDL.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio.h"
#include "misc.h"

/*
====================================================================
If audio device was properly initiated this flag is set.
If this flag is not set; no action will be taken for audio.
====================================================================
*/
int audio_ok = 0;
/*
====================================================================
If this flag is not set no sound is played.
====================================================================
*/
int audio_enabled = 1;
/*
====================================================================
Volume of all sounds
====================================================================
*/
int audio_volume = 128;

/*
====================================================================
Initiate/close audio
====================================================================
*/
int audio_open()
{
    if ( Mix_OpenAudio( MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 256 ) < 0 ) {
        fprintf( stderr, "%s\n", SDL_GetError() );
        audio_ok = 0;
        return 0;
    }
    audio_ok = 1;
    return 1;
}
void audio_close()
{
    if ( !audio_ok ) return;
    Mix_CloseAudio();
}

/*
====================================================================
Modify audio
====================================================================
*/
void audio_enable( int enable )
{
    if ( !audio_ok ) return;
    audio_enabled = enable;
    if ( !enable )
        Mix_Pause( -1 ); /* stop all sound channels */
}
void audio_set_volume( int level )
{
    if ( !audio_ok ) return;
    if ( level < 0 ) level = 0;
    if ( level > 128 ) level = 128;
    audio_volume = level;
    Mix_Volume( -1, level ); /* all sound channels */
}
void audio_fade_out( int channel, int ms )
{
    if ( !audio_ok ) return;
    Mix_FadeOutChannel( channel, ms );
}

/*
====================================================================
Wave
====================================================================
*/
Wav* wav_load( char *fname, int channel )
{
    char path[512];
    Wav *wav = 0;
    if ( !audio_ok ) return 0;
    wav = calloc( 1, sizeof( Wav ) );
    /* use SRCDIR/sounds as source directory */
    sprintf( path, "%s/sounds/%s", get_gamedir(), fname );
    wav->chunk = Mix_LoadWAV( path );
    if ( wav->chunk == 0 ) {
        fprintf( stderr, "%s\n", SDL_GetError() );
        free( wav ); wav = 0;
    }
    wav->channel = channel;
    return wav;
}
void wav_free( Wav *wav )
{
    if ( !audio_ok ) return;
    if ( wav ) {
        if ( wav->chunk )
            Mix_FreeChunk( wav->chunk );
        free( wav );
    }
}
void wav_set_channel( Wav *wav, int channel )
{
    wav->channel = channel;
}
void wav_play( Wav *wav )
{
    if ( !audio_ok ) return;
    if ( !audio_enabled ) return;
    Mix_Volume( wav->channel, audio_volume );
    Mix_PlayChannel( wav->channel, wav->chunk, 0 );
}
void wav_play_at( Wav *wav, int channel )
{
    if ( !audio_ok ) return;
    wav->channel = channel;
    wav_play( wav );
}
void wav_fade_out( Wav *wav, int ms )
{
    if ( !audio_ok ) return;
    Mix_FadeOutChannel( wav->channel, ms );
}

#endif
