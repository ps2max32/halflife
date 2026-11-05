# Half-Life on maxsdk (PS2)

Xash3D FWGS (GPLv3), the GoldSrc compatible engine, with hlsdk-portable (Valve's Half-Life 1
SDK license) as the game code, built for the PS2 with `libs/maxsdk` (clang, R5900) by
`Makefile`. The PS2's code is in `engine/platform/ps2` and `ps2/`; the engine changes are
under `XASH_PS2` and few (`common/backends.h`, `common/defaults.h`, `engine/common/launcher.c`,
`engine/common/whereami.c`, `engine/platform/platform.h`, `ref/gl/gl_opengl.c`).

```
make submodules                        the submodules the port uses (once, first)
make cxxlib                            picolibc (POSIX I/O) and libc++ for the port (once)
make [FRAMES=n] [ARGS="+map hldemo1"]  $(OUT)/halflife.elf (build/apps/halflife)
make run                               copies a valve directory (HLDATA, by default
                                       build/data/hluplink/valve) to
                                       build/pcsx2-test/hl/valve, with ps2/delta.lst and
                                       extras.pk3, and runs it in PCSX2
```

`FRAMES=n` ends the program after n drawn frames with a picture (`host:halflife.tga`) and
the test end marker; `ARGS` is added to the command line. Without `ARGS` it starts at the
menu. The Makefile's objects do not depend on its flags: `make clean` after changing them.

## Data

- Demo: Half-Life: Uplink (1999): `pak0.pak`, `gfx.wad`, `decals.wad`, `cached.wad`, the
  configs. Maps `hldemo1` .. `hldemo3` and the training course `t0a0*`. Valve's installer is
  `hluplink.exe` (archive.org); it is a Wise installer whose files are raw deflate streams.
- Retail: the `valve` directory of a retail copy (not tried yet).
- `ps2/delta.lst`: how the fields sent from server to client are encoded. Retail has one,
  Uplink not, and the engine does not start without it: `ps2/gen_delta.py` makes this one
  from `engine/common/net_encode.c`. Client and server are one program, so it only has to
  be consistent; it is generous (floats 32 bits at 1/1024) and has no custom encoders.
- `extras.pk3`: Xash3D's fonts and gamepad presets (`3rdparty/extras`), made by `make run`.

## How

| Part | How |
|---|---|
| One program | `XASH_STATIC_LIBS`: the engine, and linked in `filesystem_stdio`, `ref_gl`, `menu` (mainui), `client` and `server` (hlsdk-portable; FreeVGUI in the client), each module one relocatable object whose symbols are local but its table `lib_<name>_exports`, as waf's `scripts/waifulib/xshlib.py` does. The tables are made from the modules' objects: built with `-fvisibility=hidden`, what is left is what is `EXPORT`, for the server all its entities (`LINK_ENTITY_TO_CLASS`) |
| Submodules | upstream's commits; the PS2's changes (its platform in `build.h`) in `ps2/patches/<submodule>.patch`, applied by `ps2/patches.sh apply` before a build; after editing them `ps2/patches.sh export` |
| Platform | `XASH_PS2`, a POSIX platform with its own backends, as the PS4 (`engine/platform/ps2`); `__PS2__` from the Makefile. `engine/platform/posix` but its library loader and crash handlers |
| Renderer | `ref_gl` on `libs/libgl` (OpenGL 1.1 subset on the GS), the calls direct (`XASH_GL_STATIC`); textures up to 512 a side (libgl reports 128, but takes them: the menu's fonts and pictures need it); what libgl lacks is in `ps2/ps2_stubs.c`: behind extensions it does not report, so not called |
| Video | `engine/platform/ps2/vid_ps2.c`: libgl's TV mode, NTSC 640 x 448 or PAL 640 x 512 |
| Files | POSIX calls on the IOP file server (`host:`), `ps2/ps2_posix.c` from apps/descent3, on a picolibc with POSIX I/O; `/` is `host:`, the base directory `/hl` (`DIR`) |
| Input | pad 0 as the engine's gamepad (`in_ps2.c`): buttons `K_A_BUTTON` ..., sticks the gamepad axes, the `playstation` preset of `extras.pk3` |
| Memory | maxsdk's virtual memory (`mx_vm_config` in `sys_ps2.c`), paged to `host:halflife.swp`; built into VM (`MX_MEMORY=vm`: started by mxlink directly, or by `mxload.elf`, as `make run` does), only maxsdk, libc and libgl in physical memory; `XASH_LOW_MEMORY=1`. In PCSX2 only on the EE interpreter (`PCSX2_EE=interp`) |
| Sound | the engine's mixer at 44100 Hz 16-bit stereo into a ring, sent to maxsdk's PCM stream (`mx/snd.h`, audsrv) by `s_ps2.c` as far as it has painted |
| Network | none: the engine's POSIX code on sockets that fail (`ps2/include/ps2_net.h`), the local game on the loopback. `XASH_NO_NETWORK` no longer builds upstream |
| C++ | C++17 without exceptions: libc++ and libc++abi from the LLVM tree, built by `make cxxlib`, as apps/descent3 |
| Left out | voice chat, MP3, OGG and Opus (`nocodec_ps2.c`: Half-Life's sounds are WAV), HTTPS |

## Status

In PCSX2 (EE interpreter, VM): the menu at 99 fps; `hldemo1` plays its opening and draws the
corridor right (3350 triangles, 24000 vertices, 1.6 MB of textures in GS memory, none
evicted, 52 MB of virtual memory). The world is drawn by `ref_gl`'s `glBegin`/`glEnd`.

On the PS2 (mxlink, PAL, 16-bit colour), `FRAMES=4500 ARGS="+map hldemo1"`: the level is
connected about 30 s after the start, most of it writing to the swap file; in the world about
20 fps, up to 34 (2500 triangles, 15000 vertices). No swapping once the level runs. Start it
from a loader resident in low memory: one at 0x1E00000 is in the memory the VM hands out, and
the program crashes when its memory fills up.

Not done yet: the menu's labels are raw keys (`GameUI_NewGame`: Uplink has no
`gameui_english.txt`) and its background grey, the pad is not tried, the video modes
of the other ports (1080i, beam racing, 24-bit colour), the fast paths of the Quake ports.
