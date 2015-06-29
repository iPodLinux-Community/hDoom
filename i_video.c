/*
 * Last updated: Jun 22, 2008
 * ~Keripo
 *
 * Copyright (C) 2008 Keripo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

static const char
rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include "hotdog.h"
#include "i_video.h"

#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <linux/kd.h>
#include <sys/ioctl.h>

#include "m_swap.h"
#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"

// iPod stuff
static uint16 *ipod_screen_colour; //RGB565
static uint8 *ipod_screen_mono; //Y'UV - Y only
static uint16 *colour_palette;
static int monochrome = -1;

int IPOD_HW_VER, IPOD_LCD_TYPE;
static int IPOD_WIDTH, IPOD_HEIGHT;

static int console;
static struct termios stored_settings;

void I_ShutdownGraphics(void)
{
    close(console);
    free(ipod_screen_mono);
    free(ipod_screen_colour);
    free(colour_palette);
    HD_LCD_Quit();
}



//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?

}

static inline int ipod_get_keypress()
{
    int press = 0;
    if (read(console, &press, 1) != 1)
        return KEY_NULL;
    return press;
}

// For wheel scrolling
#define SCROLL_MOD_NUM 4 // Arbitrary number via experimentation
static int scroll_count_l = 0;
static int scroll_count_r = 0;

#define cancel_scroll() \
  scroll_count_l = 0; \
  scroll_count_r = 0

#define SCROLL_MOD_L(n) \
  ({ \
    scroll_count_r = 0; \
    int use_l = 0; \
    if (++scroll_count_l >= n) { \
      scroll_count_l -= n; \
      use_l = 1; \
    } \
    (use_l == 1); \
  })

#define SCROLL_MOD_R(n) \
  ({ \
    scroll_count_l = 0; \
    int use_r = 0; \
    if (++scroll_count_r >= n) { \
      scroll_count_r -= n; \
      use_r = 1; \
    } \
    (use_r == 1); \
  })

void I_GetEvent(void)
{
    int input;
    while ((input = ipod_get_keypress())!= KEY_NULL) {
        event_t event;
        if (KEYSTATE(input)) { // Key up/lifted
            event.type = ev_keyup;
            input = KEYCODE(input);
            switch(input) { // In numeric order for speed
                case KEY_REWIND:
                    event.data1 = KEY_LEFTARROW;
                    D_PostEvent(&event);
                    break;
                case KEY_ACTION:
                    event.data1 = ' ';
                    D_PostEvent(&event);
                    break;
                case KEY_PLAY:
                    event.data1 = KEY_RCTRL;
                    D_PostEvent(&event);
                    break;
                case KEY_FORWARD:
                    event.data1 = KEY_RIGHTARROW;
                    D_PostEvent(&event);
                    break;
                case KEY_HOLD:
                    event.data1 = KEY_ESCAPE;
                    D_PostEvent(&event);
                    break;
                case KEY_MENU: 
                    event.data1 = KEY_UPARROW;
                    D_PostEvent(&event);
                    break;
                default:
                    break;
            }
        } else {
            event.type = ev_keydown;
            input = KEYCODE(input);
            switch(input) { // In numeric order for speed
                case KEY_REWIND:
                    cancel_scroll();
                    event.data1 = KEY_LEFTARROW;
                    D_PostEvent(&event);
                    break;
                case SCROLL_R:
                    if (SCROLL_MOD_R(SCROLL_MOD_NUM)) {
                        event.data1 = '.';
                        D_PostEvent(&event);
                    } else {
                        event.type = ev_keyup;
                        event.data1 = '.';
                        D_PostEvent(&event);
                        event.data1 = ',';
                        D_PostEvent(&event);
                    }
                    break;
                case KEY_ACTION:
                    cancel_scroll();
                    event.data1 = ' ';
                    D_PostEvent(&event);
                    break;
                case KEY_PLAY:
                    cancel_scroll();
                    event.data1 = KEY_RCTRL;
                    D_PostEvent(&event);
                    break;
                case KEY_FORWARD:
                    cancel_scroll();
                    event.data1 = KEY_RIGHTARROW;
                    D_PostEvent(&event);
                    break;
                case KEY_HOLD:
                    cancel_scroll();
                    event.data1 = KEY_ESCAPE;
                    D_PostEvent(&event);
                    break;
                case SCROLL_L:
                    if (SCROLL_MOD_L(SCROLL_MOD_NUM)) {
                        event.data1 = ',';
                        D_PostEvent(&event);
                    } else {
                        event.type = ev_keyup;
                        event.data1 = '.';
                        D_PostEvent(&event);
                        event.data1 = ',';
                        D_PostEvent(&event);
                    }
                    break;
                case KEY_MENU: 
                    cancel_scroll();
                    event.data1 = KEY_UPARROW;
                    D_PostEvent(&event);
                    break;
                default:
                    break;
            }
        }
    }
}

//
// I_StartTic
//
void I_StartTic (void)
{
    I_GetEvent();
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{
    if (monochrome) { // Untested - mono owners please contact Keripo!
        int x, y, p_ipod, p_src;
        for (y = 0; y < IPOD_HEIGHT; y++) {
            for (x = 0; x < IPOD_WIDTH; x++) {
                p_ipod = x + y * IPOD_WIDTH;
                p_src = x * SCREENWIDTH / IPOD_WIDTH
                    + (y * SCREENHEIGHT / IPOD_HEIGHT) * SCREENWIDTH;
                ipod_screen_mono[p_ipod] = screens[0][p_src];
            } // ^ probably doesn't work and needs palette->RGB565->Y'UV's Y
        }
        HD_LCD_Update(ipod_screen_mono, 0, 0, IPOD_WIDTH, IPOD_HEIGHT);
    } else {
        int x, y, p_ipod, p_src;
        uint16 p1, p2, p3;
        for (y = 0; y < IPOD_HEIGHT; y++) {
            for (x = 0; x < IPOD_WIDTH; x++) {
                p_ipod = x + y * IPOD_WIDTH;
                p_src = x * SCREENWIDTH / IPOD_WIDTH
                    + (y * SCREENHEIGHT / IPOD_HEIGHT) * SCREENWIDTH;
                // Blending is cooler
                //ipod_screen_colour[p_ipod] = colour_palette[screens[0][p_src]];
                p1 = colour_palette[screens[0][p_src]];
                p2 = colour_palette[screens[0][p_src + 1]];
                p3 = colour_palette[screens[0][p_src + SCREENWIDTH]];
                ipod_screen_colour[p_ipod] = blend_pixels_4_RGB565(p1, p2, p3);
            }
        }
        HD_LCD_Update(ipod_screen_colour, 0, 0, IPOD_WIDTH, IPOD_HEIGHT);    
    }
}


//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
    if (!monochrome) {
        int i;
        for ( i=0; i<256; ++i ) {
            uint8 r, g, b;
            r = gammatable[usegamma][*palette++];
            g = gammatable[usegamma][*palette++];
            b = gammatable[usegamma][*palette++];
            colour_palette[i] = RGB565(r, g, b);
        }
    }
}

void ipod_init_input()
{
    struct termios new_settings;
    console = open("/dev/console", O_RDONLY | O_NONBLOCK);
    tcgetattr(console, &stored_settings);
    
    new_settings = stored_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_settings.c_iflag &= ~(ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON | BRKINT);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 0;
    
    tcsetattr(console, TCSAFLUSH, &new_settings);
    ioctl(console, KDSKBMODE, K_MEDIUMRAW);
}

void I_InitGraphics(void)
{
    static int        firsttime=1;
    if (!firsttime)
    return;
    firsttime = 0;

    printf("\n");
    printf("======================\n");
    printf("    hDoom 1.10 K1\n");
    printf("    for iPodLinux\n");
    printf("      by Keripo\n");
    printf("======================\n");
    printf("\n");

    ipod_init_input();
    
    HD_LCD_Init();
    HD_LCD_GetInfo(&IPOD_HW_VER, &IPOD_WIDTH, &IPOD_HEIGHT, &IPOD_LCD_TYPE);
    
    if (IPOD_LCD_TYPE == 2 || IPOD_LCD_TYPE == 3) { // monochromes (1-4G & minis)
        monochrome = 1;
        ipod_screen_mono = malloc(IPOD_WIDTH * IPOD_HEIGHT * 2);
        ipod_screen_colour = NULL;
        colour_palette = NULL;
    } else {
        monochrome = 0;
        ipod_screen_mono = NULL;
        ipod_screen_colour = malloc(IPOD_WIDTH * IPOD_HEIGHT * 2);
        colour_palette = (uint16 *)malloc(256 * sizeof(uint16));
    }
    screens[0] = (uint8 *)malloc(SCREENWIDTH * SCREENHEIGHT);
}
