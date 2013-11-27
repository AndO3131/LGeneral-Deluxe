/***************************************************************************
                          main.c  -  description
                             -------------------
    begin                : Mit Jan 17 16:03:18 CET 2001
    copyright            : (C) 2001 by Michael Speck
    email                : kulkanie@gmx.net
 ***************************************************************************/
/***************************************************************************
                     Modifications by LGD team 2012+.
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <assert.h>
#include <stdarg.h>
#include <unistd.h>

#include "lgeneral.h"
#include "parser.h"
#include "event.h"
#include "date.h"
#include "nation.h"
#include "list.h"
#include "unit.h"
#include "file.h"
#include "map.h"
#include "scenario.h"
#include "campaign.h"
#include "engine.h"
#include "player.h"
#include "localize.h"

#ifdef _GNU_SOURCE
#  undef _GNU_SOURCE
#endif
#define _GNU_SOURCE
#include <getopt.h>

/* FIXME already used twice! time to consolidate */
#ifdef __GNUC__
#  define NORETURN __attribute__ ((noreturn))
#  define PRINTF_STYLE(fmtidx, firstoptidx) __attribute__ ((format(printf,fmtidx,firstoptidx)))
#else
#  define NORETURN
#  define PRINTF_STYLE(x,y)
#endif

static void abortf(const char *fmt, ...) NORETURN PRINTF_STYLE(1, 2);
static void syntax(int argc, char *argv[]) NORETURN;

int term_game = 0;
extern Sdl sdl;
extern Config config;
extern SDL_Cursor *empty_cursor;
extern Setup setup;

/* command line stuff */
static int suppress_title;
static List *player_control;

enum Options { OPT_HELP = 256, OPT_CONTROL, OPT_SPEED, OPT_VERSION,
               OPT_CAMPAIGN_START, OPT_AI_DEBUG };

static struct option long_options[] = {
    { "campaign-start", 1, 0, OPT_CAMPAIGN_START },
    { "control", 1, 0, OPT_CONTROL },
    { "deploy-turn", 0, &config.deploy_turn, 1 },
    { "no-deploy-turn", 0, &config.deploy_turn, 0 },
    { "help", 0, 0, OPT_HELP },
    { "notitle", 0, &suppress_title, 1 },
    { "speed", 1, 0, OPT_SPEED },
    { "version", 0, 0, OPT_VERSION },
    { "purchase", 0, &config.purchase, 1 },
    { "no-purchase", 0, &config.purchase, 0 },
    { "ai-debug", 1, 0, OPT_AI_DEBUG },
    { 0, 0, 0, 0 }
};

static void show_title()
{
    int dummy;
    Font *font = 0;
    SDL_Surface *back = 0;
    if ( ( back = load_surf( search_file_name( "title", 'i' ), SDL_SWSURFACE ) ) ) {
        FULL_DEST( sdl.screen );
        FULL_SOURCE( back );
        blit_surf();
    }
    if ( ( font = load_font( search_file_name( "font_credit", 'i' ) ) ) ) {
        font->align = ALIGN_X_LEFT | ALIGN_Y_BOTTOM;
        write_text( font, sdl.screen, 2, sdl.screen->h - 14, tr("LGD (C) 2012 http://www.panzercentral.com/forum/viewtopic.php?f=98&t=48535"), 255 );
        font->align = ALIGN_X_LEFT | ALIGN_Y_BOTTOM;
        write_text( font, sdl.screen, 2, sdl.screen->h - 2, tr("LGD is a fork of LG.  LG is (C) 2001-2005 Michael Speck http://lgames.sf.net"), 255 );
    }
    refresh_screen( 0, 0, 0, 0 );
    /* wait */
    SDL_PumpEvents();
    event_clear();
    while ( !event_get_buttonup( &dummy, &dummy, &dummy ) )
    {
        SDL_PumpEvents();
        SDL_Delay( 100 );
    }
    event_clear();
    /* clear */
    free_surf(&back);
    free_font(&font);
}

/* aborts with the given error message and exit code 1 */
static void abortf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    exit(1);
}

static void syntax(int argc, char *argv[])
{
    printf(tr("Syntax: %s [options] [resource-file]\n"), argv[0]);
    printf(tr("\n"
            "Options:\n"
            "    --ai-debug=<loglev>\n"
	    "\t\tVerbosity level of debug messages for AI turn\n"
            "    --campaign-start=<state>\n"
            "\t\tInitial state to start campaign with (debug only)\n"
            "    --control=<who>\n"
            "\t\tEntity who controls the current side. <who> is either human,\n"
            "\t\tor cpu (short: h, or c). Multiple options can be specified.\n"
            "\t\tIn this case the first option refers to the first side, the\n"
            "\t\tsecond option to the second side, etc.\n"
            "    --(no-)deploy-turn\n"
            "\t\tAllow/Disallow deployment of troops before scenario starts.\n"
            "    --(no-)purchase\n"
            "\t\tEnable/Disable option for purchasing by prestige. If disabled\n"
            "\t\tthe predefined reinforcements are used.\n"
            "    --notitle\tInhibit title screen\n"
            "    --speed=<n>\tAccelerate animation speed by <n>\n"
            "    --version\tDisplay version information and quit\n"
            "\n"
            "<resource-file>\tscenario file relative to scenarios-directory,\n"
            "\t\tor campaign file relative to campaigns-directory\n"
            "\t\tunder %s\n"), get_gamedir()
          );
    exit(0);
}

static void print_version()
{
    /* display some credits */
    printf( tr("LGD %s\n"), VERSION );
    printf( tr("LGD (C) 2012 http://www.panzercentral.com/forum/viewtopic.php?f=98&t=48535\n") );
    printf( tr("LGD is a fork of LG.  LG is (C) 2001-2005 Michael Speck http://lgames.sf.net\n") );
    printf( tr("LGeneral v1.2.3\nCopyright 2001-2011 Michael Speck\nPublished under GNU GPL\n---\n") );
    printf( tr("Looking up data in: %s\n"), get_gamedir() );
#ifndef WITH_SOUND
    printf( tr("Compiled without sound and music\n") );
#endif
}

static void add_control(const char *ctlspec)
{
    int ctl = PLAYER_CTRL_NOBODY;
    assert(ctlspec);
    switch (ctlspec[0]) {
        case 'C':
        case 'c':
            if (ctlspec[1] == 0 || strcasecmp(ctlspec, "cpu") == 0) ctl = PLAYER_CTRL_CPU;
            break;
        case 'H':
        case 'h':
            if (ctlspec[1] == 0 || strcasecmp(ctlspec, "human") == 0) ctl = PLAYER_CTRL_HUMAN;
            break;
        default:
            break;
    }
    if (ctl != PLAYER_CTRL_CPU && ctl != PLAYER_CTRL_HUMAN)
        abortf(tr("Invalid player control: %s\n"), ctlspec);

    if (!player_control)
        player_control = list_create(LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK);

    list_add(player_control, (void *)ctl);
}

static void eval_cmdline(int argc, char **argv)
{
    const char *camp_scen_state = 0;
    for (;;) {
        int opt = getopt_long(argc, argv, "", long_options, 0);
        if (opt == -1) break;
        switch (opt) {
            case 0: break;	/* directly set */
            case OPT_HELP: syntax(argc, argv); break;
            case OPT_CAMPAIGN_START: camp_scen_state = optarg; break;
            case OPT_CONTROL: add_control(optarg); break;
            case OPT_SPEED:
                config.anim_speed = atoi(optarg);
                printf(tr("Animation speed up: x%d\n"),config.anim_speed);
                break;
            case OPT_AI_DEBUG:
                config.ai_debug = atoi(optarg);
                break;
            case OPT_VERSION: print_version(); exit(0);
            default: exit(1);
        }
    }

    if (optind < argc) {
        enum { LOAD_NOTHING, LOAD_SCEN, LOAD_CAMP, LOAD_GAME }
        load = LOAD_NOTHING;
        if (scen_load_info(argv[optind])) {
            setup.type = SETUP_INIT_SCEN;
            load = LOAD_SCEN;
        } else if (camp_load_info(argv[optind])) {
            setup.type = SETUP_INIT_CAMP;
            setup.scen_state = camp_scen_state;
            load = LOAD_CAMP;
        } else {
            fprintf(stderr, tr("Error loading scenario %s\n"), argv[optind]);
            exit(1);
        }
        /* imply --notitle */
        suppress_title = 1;
        if (load == LOAD_SCEN || load == LOAD_CAMP) {
            /* apply control */
            if (player_control) {
                int ctl, i;
                list_reset(player_control);
                for (i = 0; i < setup.player_count && (ctl = (int)list_next(player_control)); i++)
                    setup.ctrl[i] = ctl;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    char window_name[32];

    locale_init(0);

    /* set random seed */
    set_random_seed();

    /* check config directory path and load config */
    check_config_dir_name();
    load_config();

    eval_cmdline(argc, argv);
    print_version();

    /* init sdl */
    init_sdl( SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO );
    set_video_mode( 640, 480, 0 );
    sprintf( window_name, tr("LGeneral Deluxe %s"), VERSION );
    SDL_WM_SetCaption( window_name, 0 );
    event_enable_filter();

    /* show lgd title */
    if (!suppress_title) show_title();

    /* switch to configs resolution */
    sdl.num_vmodes = get_video_modes( &sdl.vmodes ); /* always successful */
    set_video_mode( config.width, config.height, config.fullscreen );

#ifdef WITH_SOUND
    /* initiate audio device */
    audio_open();
    audio_enable( config.sound_on );
    audio_set_volume( config.sound_volume );
#endif

    engine_create();
    SDL_SetCursor( empty_cursor );
    engine_run();
    engine_delete();

#ifdef WITH_SOUND
    /* close audio device */
    audio_close();
#endif

    /* save settings */
    save_config();

    event_disable_filter();

    if (player_control) list_delete(player_control);
    return 0;
}
