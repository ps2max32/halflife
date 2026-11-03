#!/usr/bin/env python3
"""Makes ps2/delta.lst from the engine's field tables.

delta.lst says how each field of the structures the server sends is encoded.
Retail Half-Life has one; the 1999 Uplink demo does not, and the engine
cannot start without it. Client and server are one program here, so any
consistent encoding will do: this one is generous, as bandwidth does not
matter on the loopback: floats 32 bits at 1/1024, integers at their full
size, angles 16 bits, strings as they are. No custom encoders (none).

usage: ps2/gen_delta.py > ps2/delta.lst      (from the top of the tree)

It reads the tables of engine/common/net_encode.c and compiles a small host
program that asks the compiler for each field's type.
"""
import os
import re
import subprocess
import sys
import tempfile

# the tables of net_encode.c: (file's array, macro prefix, C struct, delta.lst names)
TABLES = [
    ("ev_fields", "EVNT", "event_args_t", ["event_t"]),
    ("pm_fields", "PHYS", "movevars_t", ["movevars_t"]),
    ("cmd_fields", "UCMD", "usercmd_t", ["usercmd_t"]),
    ("cd_fields", "CLDT", "clientdata_t", ["clientdata_t"]),
    ("wd_fields", "WPDT", "weapon_data_t", ["weapon_data_t"]),
    ("ent_fields", "ENTS", "entity_state_t",
     ["entity_state_t", "entity_state_player_t", "custom_entity_state_t"]),
]

src = open("engine/common/net_encode.c").read()


def fields(array, prefix):
    """(delta.lst name, C member) of each entry of the table"""
    body = re.search(r"static const delta_field_t %s\[\] =\s*\{(.*?)\n\};" % array, src, re.S).group(1)
    out = []
    for m in re.finditer(r"%s_DEF(_?)\(\s*([^)]*?)\s*\)" % prefix, body):
        if m.group(1):  # MACRO_DEF_( name, member )
            name, member = [s.strip() for s in m.group(2).split(",")]
        else:
            name = member = m.group(2)
        out.append((name, member))
    return out


prog = ['#include "common.h"', '#include "netchan.h"', '#include "event_api.h"', '#include "entity_state.h"',
        '#include "weaponinfo.h"', '#include "event_args.h"', '#include "protocol.h"', '#include "client.h"',
        '#include <stdio.h>',
        '#define KIND(x) _Generic((x), float: "F", double: "F", char *: "S", const char *: "S", \\',
        '  signed char: "b", unsigned char: "B", char: "b", short: "h", unsigned short: "H", \\',
        '  int: "i", unsigned int: "I", long: "i", unsigned long: "I", default: "?")',
        'int main(void) {']
for array, prefix, struct, names in TABLES:
    for name, member in fields(array, prefix):
        prog.append('printf("%s %s %%s %%d\\n", KIND(((%s *)0)->%s), (int)sizeof(((%s *)0)->%s));'
                    % (array, name, struct, member, struct, member))
prog += ["return 0; }"]

inc = ["-Icommon", "-Ipublic", "-Ipm_shared", "-Iengine", "-Iengine/common", "-Iengine/client", "-Iengine/server",
       "-Iengine/common/imagelib", "-Iengine/common/soundlib", "-Iengine/client/vgui", "-Ifilesystem",
       "-I3rdparty/library_suffix/include", "-Iengine/platform"]
with tempfile.TemporaryDirectory() as d:
    c = os.path.join(d, "kinds.c")
    x = os.path.join(d, "kinds")
    open(c, "w").write("\n".join(prog))
    subprocess.run(["cc", "-std=gnu11", "-w", "-DENGINE_DLL=1", "-DXASH_LOW_MEMORY=1", "-DALLOCA_H=<alloca.h>", "-DHAVE_STRCASECMP=1",
                    "-DHAVE_STRNCASECMP=1", *inc, c, "-o", x, "-Wl,--unresolved-symbols=ignore-all"], check=True)
    kinds = subprocess.run([x], check=True, capture_output=True, text=True).stdout

kind = {}
for line in kinds.splitlines():
    array, name, k, size = line.split()
    kind[array, name] = (k, int(size))

print("// delta.lst of the PS2 port, made by ps2/gen_delta.py: see there")
for array, prefix, struct, names in TABLES:
    for table in names:
        print("%s none\n{" % table)
        for name, member in fields(array, prefix):
            k, size = kind[array, name]
            if k == "F":
                if "angles" in name:
                    enc = ("DT_ANGLE", 16, 1.0)
                else:
                    enc = ("DT_SIGNED | DT_FLOAT", 32, 1024.0)
            elif k == "?" or k == "S":  # char arrays
                enc = ("DT_STRING", 1, 1.0)
            elif size == 1:
                enc = ("DT_SIGNED | DT_BYTE" if k == "b" else "DT_BYTE", 8, 1.0)
            elif size == 2:
                enc = ("DT_SIGNED | DT_SHORT" if k == "h" else "DT_SHORT", 16, 1.0)
            else:
                enc = ("DT_SIGNED | DT_INTEGER" if k == "i" else "DT_INTEGER", 32, 1.0)
            print("\tDEFINE_DELTA( %s, %s, %d, %.1f )," % (name, enc[0], enc[1], enc[2]))
        print("}")
