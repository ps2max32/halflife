/*
s_ps2.c - PlayStation 2 (maxsdk) sound backend
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
The mixer paints 16-bit stereo at SOUND_DMA_SPEED into snd.buffer, a ring.
SNDDMA_Submit sends what it has painted since the last call (up to
snd.paintedtime) to maxsdk's PCM stream (<mx/snd.h>, audsrv on the IOP,
which plays it at 48 kHz) as far as the stream has room, and reports what
was sent as played (snd.samplepos). The sound plays the stream's ring later
(about 100 ms). There is no voice capture
*/

#include <mx/snd.h>

#include "common.h"
#include "platform/platform.h"
#include "sound.h"

#if XASH_SOUND == SOUND_PS2

#define PS2_SND_FRAMES 0x4000 // the ring: 16384 stereo frames (0.37 s), a power of two

static int ps2_sent; // stereo frames sent to the stream, as snd.paintedtime counts them

void S_Activate( qboolean active )
{
}

qboolean SNDDMA_Init( void )
{
	if( mx_snd_init( ) < 0 || mx_snd_stream_format( SOUND_DMA_SPEED, 16, 2 ) < 0 )
	{
		Con_Printf( S_ERROR "Audio: PS2 audsrv failed\n" );
		return false;
	}

	snd.format.speed    = SOUND_DMA_SPEED;
	snd.format.channels = 2;
	snd.format.width    = 2;
	snd.samples         = PS2_SND_FRAMES * 2;
	snd.buffer          = Mem_Calloc( sndpool, snd.samples * 2 );
	snd.samplepos       = 0;
	snd.initialized     = true;
	snd.backend_name    = "PS2 audsrv";
	ps2_sent = 0;

	Con_Printf( "Audio: PS2 audsrv, %d Hz 16-bit stereo\n", SOUND_DMA_SPEED );
	return true;
}

void SNDDMA_BeginPainting( void )
{
}

void SNDDMA_Submit( void )
{
	const short *ring = (const short *)snd.buffer;
	int n, room;

	if( !snd.initialized || !ring )
		return;

	if( snd.paintedtime < ps2_sent ) // the mixer chopped its counters
		ps2_sent = snd.paintedtime;

	n = snd.paintedtime - ps2_sent;
	room = (int)( mx_snd_stream_room( ) / 4 );
	if( n > room )
		n = room;

	while( n > 0 )
	{
		int at = ps2_sent & ( PS2_SND_FRAMES - 1 );
		int k = PS2_SND_FRAMES - at < n ? PS2_SND_FRAMES - at : n;
		int took = (int)( mx_snd_stream_queue( ring + at * 2, (u32)k * 4 ) / 4 );

		ps2_sent += took;
		n -= took;
		if( took < k )
			break;
	}

	// in mono samples, as the mixer counts the ring
	snd.samplepos = ( ps2_sent * 2 ) & ( snd.samples - 1 );
}

void SNDDMA_Activate( qboolean active )
{
}

void SNDDMA_Shutdown( void )
{
	Con_Printf( "Shutting down audio.\n" );
	mx_snd_stream_stop( );
	snd.initialized = false;
	if( snd.buffer )
	{
		Mem_Free( snd.buffer );
		snd.buffer = NULL;
	}
}

qboolean VoiceCapture_Init( void )
{
	return false;
}

qboolean VoiceCapture_Activate( qboolean activate )
{
	return false;
}

qboolean VoiceCapture_Lock( qboolean lock )
{
	return false;
}

void VoiceCapture_Shutdown( void )
{
}

#endif // XASH_SOUND == SOUND_PS2
