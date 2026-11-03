/*
nocodec_ps2.c - PlayStation 2 (maxsdk): voice chat and compressed sounds left out
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
The PS2 port is built without opus, vorbis and mpg123: voice chat
(client/sound/voice.c) is off, and MP3, OGG and Opus sounds and streams
don't load (WAV does: all of Half-Life's sounds are WAV)
*/

#include "common.h"
#include "client.h"
#include "voice.h"
#include "soundlib.h"

#if XASH_PS2

voice_state_t voice;
CVAR_DEFINE_AUTO( voice_loopback, "0", FCVAR_PRIVILEGED, "loopback voice back to the speaker" );

qboolean Voice_IsRecording( void ) { return false; }
void Voice_RecordStop( void ) { }
void Voice_RecordStart( void ) { }
void Voice_Disconnect( void ) { }
void Voice_StopChannel( int entnum ) { }
void Voice_LoopbackAck( void ) { }
void Voice_AddIncomingData( int ent, const byte *data, uint size, uint frames ) { }
void CL_AddVoiceToDatagram( void ) { }
void Voice_RegisterCvars( void ) { Cvar_RegisterVariable( &voice_loopback ); }
void Voice_Idle( double frametime ) { }
qboolean Voice_Init( const char *pszCodecName, int quality, qboolean preinit ) { return false; }

voice_audio_info_t Voice_GetAudioInfo( void )
{
	voice_audio_info_t info = { 0 };
	return info;
}

qboolean Sound_LoadMPG( const char *name, const byte *buffer, fs_offset_t filesize ) { return false; }
qboolean Sound_LoadOggVorbis( const char *name, const byte *buffer, fs_offset_t filesize ) { return false; }
qboolean Sound_LoadOggOpus( const char *name, const byte *buffer, fs_offset_t filesize ) { return false; }

stream_t *Stream_OpenMPG( const char *filename ) { return NULL; }
int Stream_ReadMPG( stream_t *stream, int bytes, void *buffer ) { return 0; }
int Stream_SetPosMPG( stream_t *stream, int newpos ) { return 0; }
int Stream_GetPosMPG( stream_t *stream ) { return 0; }
void Stream_FreeMPG( stream_t *stream ) { }

stream_t *Stream_OpenOggVorbis( const char *filename ) { return NULL; }
int Stream_ReadOggVorbis( stream_t *stream, int bytes, void *buffer ) { return 0; }
int Stream_SetPosOggVorbis( stream_t *stream, int newpos ) { return 0; }
int Stream_GetPosOggVorbis( stream_t *stream ) { return 0; }
void Stream_FreeOggVorbis( stream_t *stream ) { }

stream_t *Stream_OpenOggOpus( const char *filename ) { return NULL; }
int Stream_ReadOggOpus( stream_t *stream, int bytes, void *buffer ) { return 0; }
int Stream_SetPosOggOpus( stream_t *stream, int newpos ) { return 0; }
int Stream_GetPosOggOpus( stream_t *stream ) { return 0; }
void Stream_FreeOggOpus( stream_t *stream ) { }

#endif // XASH_PS2
