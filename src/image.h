/***************************************************************************
                          image.h  -  description
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

#ifndef __IMAGE_H
#define __IMAGE_H

/*
====================================================================
Surface buffer. Used to buffer parts of a surface.
====================================================================
*/
typedef struct {
    SDL_Rect buf_rect;   /* source rect in buffer */
    SDL_Rect surf_rect;  /* destination rect in surf */
    SDL_Rect old_rect;   /* refresh region of the old position if moved */
    SDL_Surface *buf;    /* actual buffer */
    SDL_Surface *surf;   /* surface buffer is assigned to */
    int hide;            /* if True this buffer is not drawn */
    int moved;           /* if this is False and move is called old_rect is
                            set to the old region */
    int recently_hidden; /* set if hidden and cleared buffer_draw */
} Buffer;

/*
====================================================================
Create buffer of size w,h with default postion x,y in surf.
SDL must have been initialized before calling this.
====================================================================
*/
Buffer* buffer_create( int w, int h, SDL_Surface *surf, int x, int y );
void buffer_delete( Buffer **buffer );

/*
====================================================================
Hide this buffer from surface (no get/draw actions)
====================================================================
*/
void buffer_hide( Buffer *buffer, int hide );

/*
====================================================================
Get buffer from buffer->surf.
====================================================================
*/
void buffer_get( Buffer *buffer );

/*
====================================================================
Draw buffer to buffer->surf.
====================================================================
*/
void buffer_draw( Buffer *buffer );

/*
====================================================================
Add refresh rects including movement and recent hide.
====================================================================
*/
void buffer_add_refresh_rects( Buffer *buffer );

/*
====================================================================
Modify the buffer settings. Does not include any get() or draw().
The maximum resize is given by buffers initial size.
====================================================================
*/
void buffer_move( Buffer *buffer, int x, int y );
void buffer_resize( Buffer *buffer, int w, int h );
void buffer_set_surface( Buffer *buffer, SDL_Surface *surf );
void buffer_get_geometry(Buffer *buffer, int *x, int *y, int *w, int *h );
SDL_Rect *buffer_get_surf_rect( Buffer *buffer );

/*
====================================================================
Image. Used to add toplevel graphics (e.g. cursor). It's either
drawn completely or partial (depends on img_rect).
====================================================================
*/
typedef struct {
    Buffer *bkgnd;          /* background buffer for image */
    SDL_Rect img_rect;      /* source rect in image */
    SDL_Surface *img;       /* image */
} Image;

/*
====================================================================
Create an image. The image surface is deleted by image_delete().
The image region (that's actually drawn) is initiated to 
0,0,buf_w,buf_h where buf_w or buw_h 0 means to use the whole image. 
The maximum region size is limited to the initial region size.
The current draw position is x,y in 'surf'.
====================================================================
*/
Image *image_create( SDL_Surface *img, int buf_w, int buf_h, SDL_Surface *surf, int x, int y );
void image_delete( Image **image );

/*
====================================================================
Hide image completly
====================================================================
*/
#define image_hide( image, hide ) buffer_hide( image->bkgnd, hide )

/*
====================================================================
Get image background
====================================================================
*/
#define image_get_bkgnd( img ) buffer_get( img->bkgnd )

/*
====================================================================
Hide the image (draw background to current position).
====================================================================
*/
#define image_draw_bkgnd( image ) buffer_draw( image->bkgnd )

/*
====================================================================
Draw image region to current position.
Stores the background first.
Add the refresh_rects as needed including movement and hide
refreshs.
====================================================================
*/
void image_draw( Image *image );

/*
====================================================================
Modify the image settings. Does not include any drawing (neither
image nor background) but may resize the background if needed.
====================================================================
*/
#define image_move( img, x, y ) buffer_move( img->bkgnd, x, y )
#define image_set_surface( img, surf ) buffer_set_surface( img->bkgnd, surf )
void image_set_region( Image *image, int x, int y, int w, int h );


/*
====================================================================
Animation. An image that changes it's draw region every 'speed'
milliseconds (next frame).
Multiple animations may be stored in one surface.
Each row represents an animation and each column #i frame number
#i of each animation.
====================================================================
*/
typedef struct {
    Image *img;     /* frame graphics */
    int playing;    /* true if an animation is playing */
    int loop;       /* true if animation is in an endless loop */
    int speed;      /* time in milliseconds a frame is displayed */
    int cur_time;   /* if this exceeds speed the next frame is played (or animation is stopped) */
} Anim;

/*
====================================================================
Create an animation. The animation surface is deleted by 
anim_delete(). The buffer size of each frame is frame_w, frame_h.
The current draw position is x,y in 'surf'.
Per default an animation is stopped and set to the first frame
of the first animation.
Animation is hidden per default.
====================================================================
*/
Anim* anim_create( SDL_Surface *gfx, int speed, int frame_w, int frame_h, SDL_Surface *surf, int x, int y );
void anim_delete( Anim **anim );

/*
====================================================================
Draw animation
====================================================================
*/
#define anim_hide( anim, hide ) buffer_hide( anim->img->bkgnd, hide )
#define anim_get_bkgnd( anim ) buffer_get( anim->img->bkgnd )
#define anim_draw_bkgnd( anim ) buffer_draw( anim->img->bkgnd )
#define anim_draw( anim ) image_draw( anim->img )

/*
====================================================================
Modify animation settings
====================================================================
*/
#define anim_move( anim, x, y ) buffer_move( anim->img->bkgnd, x, y )
#define anim_set_surface( anim, surf ) buffer_set_surface( anim->img->bkgnd, surf )
void anim_set_speed( Anim *anim, int speed );
void anim_set_row( Anim *anim, int id );
void anim_set_frame( Anim *anim, int id );
void anim_play( Anim *anim, int loop );
void anim_stop( Anim *anim );
void anim_update( Anim *anim, int ms );


/*
====================================================================
Frame. A contents surface is modified and together with a frame
it builds up the image. If alpha < 255 the background is darkened
by using shadow surface first when drawing.
====================================================================
*/
typedef struct {
    Image *img;
    SDL_Surface *shadow;
    SDL_Surface *contents;
    SDL_Surface *frame;
    int alpha;
} Frame;

/*
====================================================================
Create a frame. If alpha > 0 the background is 
shadowed by using the shadow surface when drawing.
The size of buffer, image and contents is 
specified by the measurements of the frame picture 'img'.
The current draw position is x,y in 'surf'.
====================================================================
*/
Frame *frame_create( SDL_Surface *img, int alpha, SDL_Surface *surf, int x, int y );
void frame_delete( Frame **frame );

/*
====================================================================
Draw frame
====================================================================
*/
void frame_hide( Frame *frame, int hide );
#define frame_get_bkgnd( frame ) buffer_get( frame->img->bkgnd )
#define frame_draw_bkgnd( frame ) buffer_draw( frame->img->bkgnd )
void frame_draw( Frame *frame );

/*
====================================================================
Modify/Get frame settings
====================================================================
*/
#define frame_move( frame, x, y ) buffer_move( frame->img->bkgnd, x, y )
#define frame_set_surface( frame, surf ) buffer_set_surface( frame->img->bkgnd, surf )
#define frame_get_geometry(frame,x,y,w,h) buffer_get_geometry(frame->img->bkgnd,x,y,w,h)
void frame_apply( Frame *frame ); /* apply changed contents to image */
#define frame_get_width( frame ) ( (frame)->img->img->w )
#define frame_get_height( frame ) ( (frame)->img->img->h )

#endif
