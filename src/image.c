/***************************************************************************
                          image.c  -  description
                             -------------------
    begin                : Tue Mar 21 2002
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

#include <SDL.h>
#include <stdlib.h>
#include "sdl.h"
#include "image.h"

/*
====================================================================
Create buffer of size w,h with default postion x,y in surf.
SDL must have been initialized before calling this.
====================================================================
*/
Buffer* buffer_create( int w, int h, SDL_Surface *surf, int x, int y )
{
    Buffer *buffer = 0;
    SDL_PixelFormat *format = 0;
    if ( SDL_GetVideoSurface() ) 
        format = SDL_GetVideoSurface()->format;
    else {
        fprintf( stderr, "buffer_create: video display not available\n" );
        return 0;
    }
    buffer = calloc( 1, sizeof( Buffer ) );
    buffer->buf = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, 
                                           format->BitsPerPixel,
                                           format->Rmask, format->Gmask, format->Bmask,
                                           format->Amask );
    if ( buffer->buf == 0 ) {
        fprintf( stderr, "buffer_create: %s\n", SDL_GetError() );
        free( buffer );
        return 0;
    }
    SDL_SetColorKey( buffer->buf, 0, 0 );
    buffer->buf_rect.w = buffer->surf_rect.w = buffer->old_rect.w = w;
    buffer->buf_rect.h = buffer->surf_rect.h = buffer->old_rect.h = h;
    buffer->buf_rect.x = buffer->buf_rect.y = 0;
    buffer->surf_rect.x = x; buffer->surf_rect.y = y;
    buffer->surf = surf;
    return buffer;
}
void buffer_delete( Buffer **buffer )
{
    if ( *buffer ) {
        if ( (*buffer)->buf ) SDL_FreeSurface( (*buffer)->buf );
        free( *buffer );
        *buffer = 0;
    }
}

/*
====================================================================
Hide this buffer from surface (no get/draw actions)
====================================================================
*/
void buffer_hide( Buffer *buffer, int hide )
{
    buffer->hide = hide;
    buffer->recently_hidden = hide;
}

/*
====================================================================
Get buffer from buffer->surf.
====================================================================
*/
void buffer_get( Buffer *buffer )
{
    SDL_Rect srect = buffer->surf_rect, drect = buffer->buf_rect;
    if ( !buffer->hide )
        SDL_BlitSurface( buffer->surf, &srect, buffer->buf, &drect );
}

/*
====================================================================
Draw buffer to buffer->surf. 
====================================================================
*/
void buffer_draw( Buffer *buffer )
{
    SDL_Rect drect = buffer->surf_rect, srect = buffer->buf_rect;
    if ( !buffer->hide )
        SDL_BlitSurface( buffer->buf, &srect, buffer->surf, &drect );
}

/*
====================================================================
Add refresh rects including movement and recent hide.
====================================================================
*/
void buffer_add_refresh_rects( Buffer *buffer )
{
    if ( buffer->moved ) {
        add_refresh_rect( &buffer->old_rect );
        buffer->moved = 0;
    }
    if ( !buffer->hide || buffer->recently_hidden ) {
        add_refresh_rect( &buffer->surf_rect );
        buffer->recently_hidden = 0;
    }
}

/*
====================================================================
Modify the buffer settings. Does not include any get() or draw().
The maximum resize is given by buffers initial size.
====================================================================
*/
void buffer_move( Buffer *buffer, int x, int y )
{
    if ( !buffer->moved ) {
        buffer->old_rect.x = buffer->surf_rect.x; 
        buffer->old_rect.y = buffer->surf_rect.y;
        buffer->moved = 1;
    }
    buffer->surf_rect.x = x; buffer->surf_rect.y = y;
}
void buffer_resize( Buffer *buffer, int w, int h )
{
    if ( w > buffer->buf->w ) w = buffer->buf->w;
    if ( h > buffer->buf->h ) h = buffer->buf->h;
    buffer->buf_rect.w = buffer->surf_rect.w = w;
    buffer->buf_rect.h = buffer->surf_rect.h = h;
}
void buffer_set_surface( Buffer *buffer, SDL_Surface *surf )
{
    buffer->surf = surf;
}
void buffer_get_geometry(Buffer *buffer, int *x, int *y, int *w, int *h )
{
    *x = buffer->surf_rect.x;
    *y = buffer->surf_rect.y;
    *w = buffer->surf_rect.w;
    *h = buffer->surf_rect.h;
}

/*
====================================================================
Create an image. The image surface is deleted by image_delete().
The image region (that's actually drawn) is initiated to 
0,0,buf_w,buf_h where buf_w or buw_h 0 means to use the whole image. 
The maximum region size is limited to the initial region size.
The current draw position is x,y in 'surf'.
====================================================================
*/
Image *image_create( SDL_Surface *img, int buf_w, int buf_h, SDL_Surface *surf, int x, int y )
{
    Image *image;
    if ( img == 0 ) {
        fprintf( stderr, "image_create: passed graphic is NULL: %s\n", SDL_GetError() );
        return 0;
    }
    image = calloc( 1, sizeof( Image ) );
    image->img = img;
    if ( buf_w == 0 || buf_h == 0 ) {
        buf_w = image->img->w;
        buf_h = image->img->h;
    }
    image->img_rect.w = buf_w;
    image->img_rect.h = buf_h;
    image->img_rect.x = image->img_rect.y = 0;
    if ( ( image->bkgnd = buffer_create( buf_w, buf_h, surf, x, y ) ) == 0 ) {
        SDL_FreeSurface( img );
        free( image );
        return 0;
    }
    SDL_SetColorKey( image->img, SDL_SRCCOLORKEY, 0x0 );
    return image;
}
void image_delete( Image **image )
{
    if ( *image ) {
        if ( (*image)->img ) SDL_FreeSurface( (*image)->img );
        buffer_delete( &(*image)->bkgnd );
        free( *image );
        *image = 0;
    }
}

/*
====================================================================
Draw current image region to current position.
Stores the background first.
Add the refresh_rects as needed including movement and hide
refreshs.
====================================================================
*/
void image_draw( Image *image )
{
    SDL_Rect srect = image->img_rect, drect = image->bkgnd->surf_rect;
    if ( !image->bkgnd->hide )
        SDL_BlitSurface( image->img, &srect, image->bkgnd->surf, &drect );
    buffer_add_refresh_rects( image->bkgnd );
}

/*
====================================================================
Modify the image settings. Does not include any drawing (neither
image nor background) but may resize the background if needed.
====================================================================
*/
void image_set_region( Image *image, int x, int y, int w, int h )
{
    image->img_rect.x = x;
    image->img_rect.y = y;
    buffer_resize( image->bkgnd, w, h );
    image->img_rect.w = image->bkgnd->buf_rect.w;
    image->img_rect.h = image->bkgnd->buf_rect.h;
}


/*
====================================================================
Create an animation. The animation surface is deleted by 
anim_delete(). The buffer size of each frame is frame_w, frame_h.
The current draw position is x,y in 'surf'.
Per default an animation is stopped.
Animation is hidden per default.
====================================================================
*/
Anim* anim_create( SDL_Surface *gfx, int speed, int frame_w, int frame_h, SDL_Surface *surf, int x, int y )
{
    Anim *anim = calloc( 1, sizeof( Anim ) );
    if ( ( anim->img = image_create( gfx, frame_w, frame_h, surf, x, y ) ) == 0 ) {
        free( anim );
        return 0;
    }
    anim->playing = anim->loop = 0;
    anim->speed = speed;
    anim->cur_time = 0;
    anim->img->bkgnd->hide = 1;
    return anim;
}
void anim_delete( Anim **anim )
{
    if ( *anim ) {
        image_delete( &(*anim)->img );
        free( *anim );
        *anim = 0;
    }
}

/*
====================================================================
Modify animation settings
====================================================================
*/
void anim_set_speed( Anim *anim, int speed )
{
    anim->speed = speed;
}
void anim_set_row( Anim *anim, int id )
{
    image_set_region( anim->img, anim->img->img_rect.x, anim->img->bkgnd->surf_rect.h * id, 
                      anim->img->img_rect.w, anim->img->img_rect.h );
}
void anim_set_frame( Anim *anim, int id )
{
    image_set_region( anim->img, anim->img->bkgnd->surf_rect.w * id, anim->img->img_rect.y,
                      anim->img->bkgnd->surf_rect.w, anim->img->bkgnd->surf_rect.h );
}
void anim_play( Anim *anim, int loop )
{
    anim->cur_time = 0; 
    anim->playing = 1;
    anim->loop = loop;
    anim->img->bkgnd->hide = 0;
}
void anim_stop( Anim *anim )
{
    anim->playing = 0;
    anim_set_frame( anim, 0 );
    anim->img->bkgnd->hide = 1;
}
void anim_update( Anim *anim, int ms )
{
    if ( anim->playing ) {
        anim->cur_time += ms;
        while ( anim->cur_time > anim->speed ) {
            anim->cur_time -= anim->speed;
            image_set_region( anim->img, anim->img->img_rect.x + anim->img->img_rect.w, anim->img->img_rect.y,
                              anim->img->img_rect.w, anim->img->img_rect.h );
            if ( anim->img->img_rect.x >= anim->img->img->w ) {
                if ( !anim->loop ) {
                    anim_stop( anim );
                    break;
                }
                else
                    image_set_region( anim->img, 0, anim->img->img_rect.y,
                                      anim->img->img_rect.w, anim->img->img_rect.h );
            }
        }
    }
}

/*
====================================================================
Create a frame. If alpha > 0 the background is 
shadowed by using the shadow surface when drawing.
The size of buffer, image and contents is 
specified by the measurements of the frame picture 'img'.
The current draw position is x,y in 'surf'.
====================================================================
*/
Frame *frame_create( SDL_Surface *img, int alpha, SDL_Surface *surf, int x, int y )
{
    SDL_Rect rect;
    SDL_Surface *empty_img;
    int w, h;
    Frame *frame = 0;
    SDL_PixelFormat *format = 0;
    if ( SDL_GetVideoSurface() ) 
        format = SDL_GetVideoSurface()->format;
    else {
        fprintf( stderr, "buffer_create: video display not available\n" );
        return 0;
    }
    frame = calloc( 1, sizeof( Frame ) );
    /* frame */
    if ( img == 0 ) {
        fprintf( stderr, "frame_create: passed graphic is NULL: %s\n", SDL_GetError() );
        goto failure;
    }
    frame->frame = img;
    SDL_SetColorKey( frame->frame, SDL_SRCCOLORKEY, 0x0 );
    w = frame->frame->w; h = frame->frame->h;
    /* contents */
    frame->contents = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, 
                                            format->BitsPerPixel,
                                            format->Rmask, format->Gmask, format->Bmask,
                                            format->Amask );
    if ( frame->contents == 0 ) goto sdl_failure;
    SDL_SetColorKey( frame->contents, SDL_SRCCOLORKEY, 0x0 );
    /* shadow if any transparency  */
    frame->alpha = alpha;
    if ( alpha > 0 ) {
        frame->shadow = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, 
                                              format->BitsPerPixel,
                                              format->Rmask, format->Gmask, format->Bmask,
                                              format->Amask );
        if ( frame->shadow == 0 ) goto sdl_failure;
        SDL_FillRect( frame->shadow, 0, 0x0 );
        SDL_SetColorKey( frame->shadow, SDL_SRCCOLORKEY, 0x0 );
        rect.x = 1; rect.y = 1; rect.w = frame->shadow->w - 2; rect.h = frame->shadow->h - 2;
        SDL_FillRect( frame->shadow, &rect, SDL_MapRGB( frame->shadow->format, 4, 4, 4 ) );
        SDL_SetAlpha( frame->shadow, SDL_SRCALPHA, alpha );
    }
    /* image (empty frame per default)*/
    empty_img = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, 
                                 format->BitsPerPixel,
                                 format->Rmask, format->Gmask, format->Bmask,
                                 format->Amask );
    if ( empty_img == 0 ) goto sdl_failure;
    SDL_FillRect( empty_img, 0, 0x0 );
    SDL_SetColorKey( empty_img, SDL_SRCCOLORKEY, 0x0 );
    if ( ( frame->img = image_create( empty_img, 0, 0, surf, x, y ) ) == 0 ) goto failure;
    frame_apply( frame );
    return frame;
sdl_failure:
    fprintf( stderr, "frame_create: %s\n", SDL_GetError() );
failure:
    frame_delete( &frame );
    return 0;
}
void frame_delete( Frame **frame )
{
    if ( *frame ) {
        if ( (*frame)->contents ) SDL_FreeSurface( (*frame)->contents );
        if ( (*frame)->shadow ) SDL_FreeSurface( (*frame)->shadow );
        if ( (*frame)->frame ) SDL_FreeSurface( (*frame)->frame );
        image_delete( &(*frame)->img );
        free( *frame ); *frame = 0;
    }
}

/*
====================================================================
Draw frame
====================================================================
*/
void frame_hide( Frame *frame, int hide )
{
    image_hide( frame->img, hide );
}
void frame_draw( Frame *frame )
{
    SDL_Rect drect = frame->img->bkgnd->surf_rect;
    if ( !frame->img->bkgnd->hide && frame->alpha < 255 )
        SDL_BlitSurface( frame->shadow, 0, frame->img->bkgnd->surf, &drect );
    image_draw( frame->img );
}

/*
====================================================================
Modify frame settings
====================================================================
*/
void frame_apply( Frame *frame )
{
    SDL_FillRect( frame->img->img, 0, 0x0 );
    SDL_BlitSurface( frame->frame, 0, frame->img->img, 0 );
    SDL_BlitSurface( frame->contents, 0, frame->img->img, 0 );
}
