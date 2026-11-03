/*
in_ps2.c - PlayStation 2 (maxsdk) input backend
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
Pad 0 (maxsdk's <mx/pad.h>) is the engine's gamepad: the buttons are the
gamepad keys (K_A_BUTTON ...) with the common gamepad bindings, the sticks
the gamepad axes. It is polled once a frame in Platform_RunEvents. As
platform/ps4/in_ps4.c
*/

#include <mx/pad.h>

#include "platform/platform.h"
#include "input.h"
#include "client.h"

static const struct
{
	uint32_t mask;
	int key;
} ps2_buttons[] =
{
{ MX_PAD_CROSS,    K_A_BUTTON },
{ MX_PAD_CIRCLE,   K_B_BUTTON },
{ MX_PAD_SQUARE,   K_X_BUTTON },
{ MX_PAD_TRIANGLE, K_Y_BUTTON },
{ MX_PAD_L1,       K_L1_BUTTON },
{ MX_PAD_R1,       K_R1_BUTTON },
{ MX_PAD_L2,       K_L2_BUTTON },
{ MX_PAD_R2,       K_R2_BUTTON },
{ MX_PAD_L3,       K_LSTICK },
{ MX_PAD_R3,       K_RSTICK },
{ MX_PAD_START,    K_START_BUTTON },
{ MX_PAD_SELECT,   K_BACK_BUTTON },
{ MX_PAD_UP,       K_DPAD_UP },
{ MX_PAD_DOWN,     K_DPAD_DOWN },
{ MX_PAD_LEFT,     K_DPAD_LEFT },
{ MX_PAD_RIGHT,    K_DPAD_RIGHT },
};

static struct
{
	qboolean opened;
	uint32_t buttons;
	short    axes[MAX_AXES];
	double   repeat_time; // when a held D-pad button repeats next, 0 - none
} ps2pad;

// the pad doesn't repeat buttons like a keyboard, menus (sliders, lists) need it
#define PS2_REPEAT_DELAY    0.4
#define PS2_REPEAT_INTERVAL 0.08
#define PS2_REPEAT_BUTTONS  ( MX_PAD_UP | MX_PAD_DOWN | MX_PAD_LEFT | MX_PAD_RIGHT )

static short PS2_StickToAxis( uint8_t value )
{
	// 0..255 to -32768..32767, 128 is center
	int v = ((int)value - 128 ) * 256;

	return (short)bound( -32768, v, 32767 );
}

static void PS2_SendAxis( engineAxis_t axis, short value )
{
	if( ps2pad.axes[axis] == value )
		return;

	ps2pad.axes[axis] = value;
	Joy_AxisMotionEvent( axis, value );
}

static void PS2_PadUpdate( void )
{
	uint32_t buttons, changed;
	uint8_t st[4]; // right x, y, left x, y

	if( !ps2pad.opened )
		return;

	buttons = mx_pad_buttons( 0 );
	changed = buttons ^ ps2pad.buttons;

	for( size_t i = 0; changed && i < ARRAYSIZE( ps2_buttons ); i++ )
	{
		if( FBitSet( changed, ps2_buttons[i].mask ))
			Key_Event( ps2_buttons[i].key, FBitSet( buttons, ps2_buttons[i].mask ) ? true : false );
	}

	// repeat held D-pad buttons in menus and console, not in the game
	if( changed & PS2_REPEAT_BUTTONS )
		ps2pad.repeat_time = ( buttons & PS2_REPEAT_BUTTONS ) ? host.realtime + PS2_REPEAT_DELAY : 0.0;

	if( cls.key_dest != key_game && ps2pad.repeat_time > 0.0 && host.realtime >= ps2pad.repeat_time )
	{
		for( size_t i = 0; i < ARRAYSIZE( ps2_buttons ); i++ )
		{
			if( FBitSet( ps2_buttons[i].mask, PS2_REPEAT_BUTTONS ) && FBitSet( buttons, ps2_buttons[i].mask ))
				Key_Event( ps2_buttons[i].key, true );
		}

		ps2pad.repeat_time = host.realtime + PS2_REPEAT_INTERVAL;
	}

	ps2pad.buttons = buttons;

	// the analog sticks, if the pad is in analog mode
	if( mx_pad_sticks( 0, st ))
	{
		PS2_SendAxis( JOY_AXIS_SIDE, PS2_StickToAxis( st[2] ));
		PS2_SendAxis( JOY_AXIS_FWD, PS2_StickToAxis( st[3] ));
		PS2_SendAxis( JOY_AXIS_YAW, PS2_StickToAxis( st[0] ));
		PS2_SendAxis( JOY_AXIS_PITCH, PS2_StickToAxis( st[1] ));
	}
}

void Platform_RunEvents( void )
{
	PS2_PadUpdate( );
}

void Platform_MouseMove( float *x, float *y )
{
	*x = *y = 0.0f;
}

int Platform_JoyInit( void )
{
	if( ps2pad.opened )
		return 1;

	if( mx_pad_init( ) < 0 )
	{
		Con_Printf( S_ERROR "Joystick: PS2 pad driver failed\n" );
		return 0;
	}

	Con_Reportf( "Joystick: PS2 pad 0\n" );
	ps2pad.opened = true;
	ps2pad.buttons = 0;
	memset( ps2pad.axes, 0, sizeof( ps2pad.axes ));

	Joy_SetCapabilities( false, 0, "playstation" );
	return 1;
}

void Platform_JoyShutdown( void )
{
}

void Platform_CalibrateGamepadGyro( void )
{
}

key_modifier_t Platform_GetKeyModifiers( void )
{
	return KeyModifier_None;
}
