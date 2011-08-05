/***************************************************************************
                          sdl.h  -  description
                             -------------------
    begin                : Thu Apr 20 2000
    copyright            : (C) 2000 by Michael Speck
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

#ifndef SDL_H
#define SDL_H

#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/* for development */
//#define SDL_1_1_5

// draw region //
#define DEST(p, i, j, k, l) {sdl.d.s = p; sdl.d.r.x = i; sdl.d.r.y = j; sdl.d.r.w = k; sdl.d.r.h = l;}
#define SOURCE(p, i, j) {sdl.s.s = p; sdl.s.r.x = i; sdl.s.r.y = j; sdl.s.r.w = sdl.d.r.w; sdl.s.r.h = sdl.d.r.h;}
#define FULL_DEST(p) {sdl.d.s = p; sdl.d.r.x = 0; sdl.d.r.y = 0; sdl.d.r.w = (p)->w; sdl.d.r.h = (p)->h;}
#define FULL_SOURCE(p) {sdl.s.s = p; sdl.s.r.x = 0; sdl.s.r.y = 0; sdl.s.r.w = sdl.d.r.w; sdl.s.r.h = sdl.d.r.h;}
typedef struct {
    SDL_Surface *s;
    SDL_Rect    r;
} DrawRgn;

// Sdl Surface //
#define SDL_NONFATAL 0x10000000
SDL_Surface* load_surf(const char *fname, int f);
SDL_Surface* create_surf(int w, int h, int f);
void free_surf( SDL_Surface **surf );
int  disp_format(SDL_Surface *sur);
inline void lock_surf(SDL_Surface *sur);
inline void unlock_surf(SDL_Surface *sur);
void blit_surf(void);
void alpha_blit_surf(int alpha);
void fill_surf(int c);
void set_surf_clip( SDL_Surface *surf, int x, int y, int w, int h );
Uint32 set_pixel( SDL_Surface *surf, int x, int y, int pixel );
Uint32 get_pixel( SDL_Surface *surf, int x, int y );

// Sdl Font //
#ifdef SDL_1_1_5
enum {
    OPAQUE = 0
};
#else
enum {
    OPAQUE = 255
};
#endif
enum {
    ALIGN_X_LEFT	= (1L<<1),
    ALIGN_X_CENTER	= (1L<<2),
    ALIGN_X_RIGHT	= (1L<<3),
    ALIGN_Y_TOP	    = (1L<<4),
    ALIGN_Y_CENTER	= (1L<<5),
    ALIGN_Y_BOTTOM	= (1L<<6)
};

typedef struct _Font {
    SDL_Surface *pic;
    int         align;
    int         color;
    int         height;
    int         char_offset[256];
    int		width;	// total width of surface, *must* follow char_offset
    			// because it is used for spilling char_offset
    char        keys[256];
    char        offset;
    char        length;
    //last written rect
    int     	last_x;
    int         last_y;
    int	        last_width;
    int	        last_height;
} Font;
Font* load_font(const char *fname);
void font_load_glyphs(Font *font, const char *fname);
void free_font(Font **sfnt);
int  write_text(Font *sfnt, SDL_Surface *dest, int x, int y, const char *str, int alpha);
void write_line( SDL_Surface *surf, Font *font, const char *str, int x, int *y );
inline void lock_font(Font *sfnt);
inline void unlock_font(Font *sfnt);
SDL_Rect last_write_rect(Font *fnt);
int  text_width(Font *fnt, const char *str);
int  char_width(Font *fnt, char c);

/* Sdl */
enum {
    RECT_LIMIT = 200,
    DIM_STEPS = 8,
    DIM_DELAY = 20
};
#define DIM_SCREEN()   dim_screen(DIM_STEPS, DIM_DELAY, 255)
#define UNDIM_SCREEN() undim_screen(DIM_STEPS, DIM_DELAY, 255)
typedef struct {
	int width, height, depth;
	int fullscreen;
} VideoModeInfo;
typedef struct {
    SDL_Surface *screen;
    DrawRgn     d, s;
    int         rect_count;
    SDL_Rect    rect[RECT_LIMIT];
    int         num_vmodes;
    VideoModeInfo *vmodes;
} Sdl;
void init_sdl( int f );
void quit_sdl();
int get_video_modes( VideoModeInfo **vmi );
int  set_video_mode( int width, int height, int fullscreen );
void hardware_cap();
void refresh_screen( int x, int y, int w, int h );
void refresh_rects();
void add_refresh_region( int x, int y, int w, int h );
void add_refresh_rect( SDL_Rect *rect );
void dim_screen(int steps, int delay, int trp);
void undim_screen(int steps, int delay, int trp);
int  wait_for_key();
void wait_for_click();
inline void lock_screen();
inline void unlock_screen();
inline void flip_screen();

/* cursor */
/* creates cursor */
SDL_Cursor* create_cursor( int width, int height, int hot_x, int hot_y, const char *source );

/* timer */
int get_time();
void reset_timer();

#ifdef __cplusplus
};
#endif

#endif
