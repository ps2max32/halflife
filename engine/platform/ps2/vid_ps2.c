/*
vid_ps2.c - PlayStation 2 (maxsdk) video backend
Copyright (C) 2026 Xash3D FWGS contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

/*
The screen is ps2max's libgl (an OpenGL 1.1 subset on the GS and VU1), in
the console's TV mode: NTSC 640x448 or PAL 640x512. ref_gl is built with
XASH_GL_STATIC: it calls libgl directly. The size is fixed, there are no
windows
*/

#include <mxgl.h>

#include "platform/platform.h"
#include "client.h"
#include "vid_common.h"
#include "runcfg.h" // PS2_FRAMES, see the Makefile

static int ps2_dummy_window; // host.hWnd must be non-NULL while the screen is open
static qboolean ps2_opened;

void Platform_Minimize_f( void )
{
}

platform_orientation_t Platform_GetDisplayOrientation( void )
{
	return ORIENTATION_LANDSCAPE;
}

// software renderer isn't supported
qboolean SW_CreateBuffer( int width, int height, uint *stride, uint *bpp, uint *r, uint *g, uint *b )
{
	return false;
}

void *SW_LockBuffer( void )
{
	return NULL;
}

void SW_UnlockBuffer( void )
{
}

int R_MaxVideoModes( void )
{
	return 0;
}

vidmode_t *R_GetVideoMode( int num )
{
	return NULL;
}

void *GL_GetProcAddress( const char *name )
{
	return (void *)mxglGetProcAddress( name );
}

void GL_UpdateSwapInterval( void )
{
	// libgl always waits for the vertical blank
	ClearBits( gl_vsync.flags, FCVAR_CHANGED );
}

void GL_SwapBuffers( void )
{
	static int frames;

	mxgl_end_frame();
	mxgl_frame_done();

	// a test run (FRAMES=n): its picture after n frames, then the end
	if( PS2_FRAMES && ++frames >= PS2_FRAMES )
	{
		int r = mxgl_screenshot( "halflife.tga" );
		Con_Printf( "ps2: %d frames, picture host:halflife.tga: %d\n", frames, r );
		exit( 0 );
	}
}

int GL_SetAttribute( int attr, int val )
{
	return 0;
}

int GL_GetAttribute( int attr, int *val )
{
	switch( attr )
	{
	case REF_GL_RED_SIZE:
	case REF_GL_GREEN_SIZE:
	case REF_GL_BLUE_SIZE: *val = 5; return 0;
	case REF_GL_ALPHA_SIZE: *val = 0; return 0;
	case REF_GL_DEPTH_SIZE: *val = 24; return 0;
	case REF_GL_DOUBLEBUFFER: *val = 1; return 0;
	case REF_GL_CONTEXT_MAJOR_VERSION: *val = 1; return 0;
	case REF_GL_CONTEXT_MINOR_VERSION: *val = 1; return 0;
	}

	*val = 0;
	return 0;
}

ref_window_type_t R_GetWindowHandle( void **handle, ref_window_type_t type )
{
	return REF_WINDOW_TYPE_NULL;
}

/*
==================
VID_SetMode

Opens the screen in the TV's mode, its size is the size of the screen
==================
*/
qboolean VID_SetMode( void )
{
	int w, h;

	if( ps2_opened )
		return true;

	mxgl_init( );
	if( mxgl_set_mode( MXGL_SD ) < 0 )
		return false;
	mxgl_screen( &w, &h );

	ps2_opened = true;
	host.hWnd = (void *)&ps2_dummy_window;
	refState.window_mode = WINDOW_MODE_FULLSCREEN;
	VID_SetDisplayTransform( &w, &h );
	R_SaveVideoMode( w, h, w, h, false );
	SetBits( gl_vsync.flags, FCVAR_CHANGED );

	Con_Printf( "Video: " S_GREEN "PS2 libgl" S_DEFAULT ", %dx%d (%s)\n", w, h,
		mxgl_mode( ) == MXGL_PAL ? "PAL" : "NTSC" );
	return true;
}

rserr_t R_ChangeDisplaySettings( int width, int height, window_mode_t window_mode )
{
	// the screen size is fixed
	return VID_SetMode() ? rserr_ok : rserr_invalid_context;
}

qboolean R_Init_Video( ref_graphic_apis_t type )
{
	if( type != REF_GL )
	{
		Con_Printf( S_ERROR "%s: only OpenGL renderers are supported\n", __func__ );
		return false;
	}

	refState.desktopBitsPixel = 16;
	glw_state.software = false;

	if( !VID_SetMode( ))
		return false;

	ref.dllFuncs.GL_InitExtensions();
	host.renderinfo_changed = false;
	return true;
}

void R_Free_Video( void )
{
	host.hWnd = NULL;
	ref.dllFuncs.GL_ClearExtensions();
}

void VID_Info_f( void )
{
	Con_Printf( "Video: " S_GREEN "PS2 libgl" S_DEFAULT ", %dx%d\n", refState.width, refState.height );
}
