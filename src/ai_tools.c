/***************************************************************************
                          ai_tools.c  -  description
                             -------------------
    begin                : Sat Oct 5 2002
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

#include "lgeneral.h"
#include "unit.h"
#include "map.h"
#include "ai_tools.h"

extern Mask_Tile **mask;

/*
====================================================================
Check the surrounding of a tile and apply the eval function to
it. Results may be stored in the context.
If 'eval_func' returns False the evaluation is broken up.
====================================================================
*/
static AI_Pos *ai_create_pos( int x, int y )
{
    AI_Pos *pos = calloc( 1, sizeof( AI_Pos ) );
    pos->x = x; pos->y = y;
    return pos;
}
void ai_eval_hexes( int x, int y, int range, int(*eval_func)(int,int,void*), void *ctx )
{
    List *list = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
    AI_Pos *pos;
    int i, nx, ny;
    /* gather and handle all positions by breitensuche.
       use AUX mask to mark visited positions */
    map_clear_mask( F_AUX );
    list_add( list, ai_create_pos( x, y ) ); mask[x][y].aux = 1;
    list_reset( list );
    while ( list->count > 0 ) {
        pos = list_dequeue( list );
        if ( !eval_func( pos->x, pos->y, ctx ) ) {
            free( pos );
            break;
        }
        for ( i = 0; i < 6; i++ )
            if ( get_close_hex_pos( pos->x, pos->y, i, &nx, &ny ) )
                if ( !mask[nx][ny].aux && get_dist( x, y, nx, ny ) <= range ) {
                    list_add( list, ai_create_pos( nx, ny ) );
                    mask[nx][ny].aux = 1;
                }
        free( pos );
    }
    list_delete( list );
}

