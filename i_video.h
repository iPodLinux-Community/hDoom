// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __I_VIDEO__
#define __I_VIDEO__


#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif


// Called by D_DoomMain,
// determines the hardware configuration
// and sets up the video mode
void I_InitGraphics (void);


void I_ShutdownGraphics(void);

// Takes full 8 bit values.
void I_SetPalette (byte* palette);

void I_UpdateNoBlit (void);
void I_FinishUpdate (void);

// Wait for vertical retrace or pause a bit.
void I_WaitVBL(int count);

void I_ReadScreen (byte* scr);

void I_BeginRead (void);
void I_EndRead (void);


// ===== iPodLinux stuff ===== //

// hotdog calls
extern void HD_LCD_Init();
extern void HD_LCD_GetInfo (int *hw_ver, int *lcd_width, int *lcd_height, int *lcd_type);
extern void HD_LCD_Update (void *fb, int x, int y, int w, int h);
extern void HD_LCD_Quit();


// Key input stuff

#define KEY_MENU    50 // Up
#define KEY_PLAY    32 // Down
#define KEY_REWIND  17 // Left
#define KEY_FORWARD 33 // Right
#define KEY_ACTION  28 // Select
#define KEY_HOLD    35 // Exit
#define SCROLL_L    38 // Counter-clockwise
#define SCROLL_R    19 // Clockwise
#define KEY_NULL    -1 // No key event

#define KEYCODE(a)  (a & 0x7f) // Use to get keycode of scancode.
#define KEYSTATE(a) (a & 0x80) // Check if key is pressed or lifted

#define inl(a) \
    (*(volatile unsigned int *)(a)) \


// Pixels

#define get_R_from_RGB565(p) \
	(((p) & 0xf800) >> 8)

#define get_G_from_RGB565(p) \
	(((p) & 0x07e0) >> 3)

#define get_B_from_RGB565(p) \
	(((p) & 0x001f) << 3)

#define RGB565(r, g, b) \
	((r >> 3) << 11) | ((g >> 2) << 5) | ((b >> 3) << 0)

// Blend four pixel values - p1 gets weighted two times
#define get_avg_R_from_4_RGB565(p1, p2, p3) \
	(((get_R_from_RGB565(p1) << 1) + get_R_from_RGB565(p2) + get_R_from_RGB565(p3)) >> 2)

#define get_avg_G_from_4_RGB565(p1, p2, p3) \
	(((get_G_from_RGB565(p1) << 1) + get_G_from_RGB565(p2) + get_G_from_RGB565(p3)) >> 2)

#define get_avg_B_from_4_RGB565(p1, p2, p3) \
	(((get_B_from_RGB565(p1) << 1) + get_B_from_RGB565(p2) + get_B_from_RGB565(p3)) >> 2)

#define blend_pixels_4_RGB565(p1, p2, p3) \
	RGB565( \
		get_avg_R_from_4_RGB565(p1, p2, p3), \
		get_avg_G_from_4_RGB565(p1, p2, p3), \
		get_avg_B_from_4_RGB565(p1, p2, p3) \
	)	

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
