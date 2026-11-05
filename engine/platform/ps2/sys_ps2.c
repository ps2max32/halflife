/*
sys_ps2.c - PlayStation 2 (maxsdk) system backend
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
The PS2 port (ps2max's maxsdk, see ps2/ and the Makefile): the engine, the
renderer, the filesystem, the menu and the game libraries are linked into
one program (XASH_STATIC_LIBS). The files are on the IOP file server
(host:), through the POSIX calls of ps2/ps2_posix.c. Time is the EE's
cycle counter
*/

#include <mx/mx.h>
#include <mx/vm.h>

#include "platform/platform.h"

#if XASH_TIMER == TIMER_PS2
// the cycle counter (294.912 MHz) wraps every 14.5 s: counted on in 64 bits
double Platform_DoubleTime( void )
{
	static uint32_t last, high;
	static uint64_t base;
	uint32_t c = mx_count();
	uint64_t now;

	if( c < last )
		high++;
	last = c;
	now = (uint64_t)high << 32 | c;

	if( !base )
		base = now;

	return (double)( now - base ) / MX_EE_HZ;
}
#endif // XASH_TIMER == TIMER_PS2

void Platform_Sleep( int msec )
{
	// one thread: the frame waits by drawing the next one
}

// the build's settings: PS2_ARGS (more of the command line), PS2_DIR (the
// base directory on the file server), see the Makefile
#include "runcfg.h"

/*
==================
PS2_GetArgv

The command line: the program's name, the PS2's defaults, then PS2_ARGS;
the base directory (XASH3D_BASEDIR) is PS2_DIR, "/hl" (host:hl), its game
directories below it (valve)
==================
*/
int PS2_GetArgv( int in_argc, char **in_argv, char ***out_argv )
{
	static char line[1024] = "xash3d -dev 1 -console " PS2_ARGS;
	static char *args[64];
	int n = 0;

	for( char *p = strtok( line, " " ); p && n < 63; p = strtok( NULL, " " ))
		args[n++] = p;
	args[n] = NULL;

	setenv( "XASH3D_BASEDIR", PS2_DIR, 1 );

	*out_argv = args;
	return n;
}

// maxsdk's virtual memory (<mx/vm.h>): malloc in up to 128 MiB, paged to a
// swap file on the file server; started by mxload.elf (MX_MEMORY=vm)
#if PS2_VM
const struct mx_vm_config mx_vm_config = {
	.swap = "halflife.swp",
};
#endif

void PS2_Init( void )
{
	Con_Printf( "PS2: virtual memory %s\n", mx_vm_active( ) ? "on" : "off" );
}

void PS2_Shutdown( void )
{
}
