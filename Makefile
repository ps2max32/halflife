# Half-Life (Xash3D FWGS with hlsdk-portable) on maxsdk: the PS2 port.
#
#   make submodules      the submodules the port uses (once, before anything else:
#                        the source lists below look at them)
#   make cxxlib          picolibc (with POSIX I/O) and libc++ for this port,
#                        into $(LIBOUT) (once; the ELF depends on them)
#   make [OUT=<dir>]     $(OUT)/halflife.elf
#   make run             halflife.elf in PCSX2 with the game data in
#                        build/pcsx2-test/hl/valve (HLDATA: a valve directory,
#                        the Uplink demo's or a retail one, copied there once)
#
# One program (XASH_STATIC_LIBS): the engine, and linked in its modules, each
# made into one relocatable object whose symbols are local but its export
# table lib_<name>_exports (as waf's scripts/waifulib/xshlib.py does):
# filesystem_stdio, ref_gl (the renderer, calling ps2max's libgl directly:
# XASH_GL_STATIC), menu (mainui), client and server (hlsdk-portable, with
# FreeVGUI in the client). The PS2's platform code is engine/platform/ps2;
# ps2/ps2_posix.c puts the POSIX file calls on the IOP file server (the PS2
# is a POSIX platform for Xash, as the PS4; ps2/include/ps2_net.h: sockets
# that fail, no network). The
# submodules are upstream's, ps2/patches/<submodule>.patch is applied to
# them before the build (ps2/patches.sh). No voice, no MP3/OGG.

HERE     := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
PS2MAX   ?= $(abspath $(HERE)/../..)
MAXSDK   ?= $(PS2MAX)/libs/maxsdk
MAXSDK_OUT ?= $(PS2MAX)/build/libs/maxsdk
LIBOUT   := $(PS2MAX)/build/apps/halflife
MX_MEMORY ?= vm
OUT      ?= $(LIBOUT)
all: $(OUT)/halflife.elf
MXGL     ?= $(PS2MAX)/libs/libgl
include $(MXGL)/mk/mxgl.mk
include $(MAXSDK)/mk/maxsdk.mk
# MX_MEMORY=vm (started by mxload.elf): the game in VM, libgl in core
MX_CORE_OBJS = $(MXGL_OBJS)

LLVM_SRC ?= $(PS2MAX_LLVM_SRC)
PICOLIBC_SRC ?= $(PS2MAX)/libs/picolibc
ZLIB_SRC ?= $(PS2MAX)/libs/zlib
# this port's picolibc (POSIX I/O) and libc++, as apps/descent3's
PICOLIBC := $(LIBOUT)/picolibc/install
LIBCXX   := $(LIBOUT)/libcxx/install
PICOLIBC_A := $(PICOLIBC)/lib/libc.a
LIBCXX_A   := $(LIBCXX)/lib/libc++.a
CXX      := $(LLVM)/clang++
LD_R     := $(LLVM)/ld.lld -m elf32ltsmipn32 -r
OBJCOPY  := $(LLVM)/llvm-objcopy

LIBRT    = $(MAXSDK_OUT)/librt.a
# libmxc's malloc (TLSF), not picolibc's (apps/descent3)
LIBMXC_OBJS = $(OUT)/libc/glue.o $(MAXSDK_OUT)/libc/compat.c.o $(MAXSDK_OUT)/libc/atomic.c.o \
              $(MAXSDK_OUT)/libc/malloc.c.o
LIBC_LIBS = $(LIBMXC_OBJS) $(LIBCXX)/lib/libc++.a $(LIBCXX)/lib/libc++abi.a $(PICOLIBC_A) \
            $(MX_LIB) $(LIBRT)

TARGET   := --target=mips64el -mcpu=r5900 -mabi=n32 -fno-pic -mno-abicalls -G0
SYSINC   := -isystem $(HERE)/ps2/include -isystem $(PICOLIBC)/include
LIB_FLAGS := $(TARGET) -nostdlibinc $(SYSINC) -ffunction-sections -fdata-sections -D_GNU_SOURCE -D_POSIX_TIMERS=1 -D_POSIX_MONOTONIC_CLOCK=200112L \
             -Wno-c++11-narrowing

OPT ?= -O2
# all of the code: the PS2 platform, static modules, low memory, no network;
# float math fast (the FPU has no doubles)
COMMON   := $(TARGET) $(OPT) -fno-common -ffunction-sections -fdata-sections -MMD -w \
            -ffinite-math-only -fno-math-errno -ffast-math -fapprox-func -mvu0 \
            -D__PS2__ -D_GNU_SOURCE -D_POSIX_TIMERS=1 -D_POSIX_MONOTONIC_CLOCK=200112L -DNDEBUG -DXASH_STATIC_LIBS=1 -DXASH_LOW_MEMORY=1 \
            -DXASH_GAMEDIR=\"valve\" -DXASH_REF_GL_ENABLED=1 \
            -DHAVE_STRCASECMP=1 -DHAVE_STRNCASECMP=1 -DHAVE_STRCASESTR=1 -DHAVE_STRCHRNUL=1 \
            -DHAVE_STRLCPY=1 -DHAVE_STRLCAT=1 -DHAVE_STRNLEN=1 -DHAVE_TGMATH_H=1 \
            '-DALLOCA_H=<alloca.h>' \
            -I$(OUT)/gen
CFLAGS_C   := $(COMMON) -std=gnu11 -nostdlibinc $(SYSINC)
CFLAGS_CXX := $(COMMON) -std=gnu++17 -fno-exceptions -nostdinc++ -nostdlibinc \
              -isystem $(LIBCXX)/include/c++/v1 $(SYSINC)

# ---- sources ------------------------------------------------------------------------
X        := $(HERE)
LS       := $(X)/3rdparty/library_suffix
HL       := $(X)/3rdparty/hlsdk-portable
MENU     := $(X)/3rdparty/mainui
VGUI     := $(HL)/freevgui
SDKINC   := -I$(X)/common -I$(X)/public -I$(X)/pm_shared -I$(X)/engine -I$(LS)/include
src = $(patsubst $(X)/%,%,$(wildcard $(addprefix $(X)/,$(1))))

# public, library_suffix and zlib: in the engine and in every module that uses them
PUBLIC_C := $(filter-out public/build_vcs.c,$(call src,public/*.c)) 3rdparty/library_suffix/src/library_suffix.c
ZLIB_C   := adler32 compress crc32 deflate inffast inflate inftrees trees uncompr zutil

ENGINE_C := $(filter-out engine/common/http/net_http_tls.c, \
              $(call src,engine/common/*.c engine/common/imagelib/*.c engine/common/soundlib/*.c \
                engine/server/*.c engine/common/http/*.c)) \
            $(filter-out engine/client/soundlib/snd_mp3.c engine/client/soundlib/snd_ogg_opus.c \
                engine/client/soundlib/snd_ogg_vorbis.c engine/client/soundlib/ogg_filestream.c \
                engine/client/sound/voice.c, \
              $(call src,engine/client/*.c engine/client/*/*.c engine/client/*/*/*.c)) \
            $(call src,engine/platform/ps2/*.c) engine/platform/misc/lib_static.c \
            $(addprefix engine/platform/posix/,sys_posix.c con_posix.c id_posix.c) \
            public/build_vcs.c
BZIP2_C  := $(addprefix 3rdparty/bzip2/bzip2/,blocksort.c huffman.c crctable.c randtable.c compress.c \
              decompress.c bzlib.c)
MEMU_C   := $(call src,3rdparty/MultiEmulator/src/*.c)
ENGINE_INC := -I$(X)/engine -I$(X)/engine/common -I$(X)/engine/common/imagelib -I$(X)/engine/server \
              -I$(X)/engine/client -I$(X)/engine/client/vgui -I$(X)/engine/common/soundlib \
              -I$(X)/engine/platform -I$(X)/filesystem $(SDKINC) -I$(ZLIB_SRC) \
              -I$(X)/3rdparty/bzip2/bzip2 -I$(X)/3rdparty/MultiEmulator/include -I$(MXGL_INC) -I$(MAXSDK)/include

FS_C     := $(call src,filesystem/*.c filesystem/*.cpp)
REF_C    := $(call src,ref/gl/*.c ref/gl/gl2_shim/*.c ref/common/*.c)
REF_INC  := -I$(X)/ref/gl -I$(X)/ref/common -I$(X)/engine -I$(X)/engine/common -I$(X)/filesystem $(SDKINC)

MENU_CXX := $(call src,$(addprefix 3rdparty/mainui/,*.cpp miniutl/*.cpp font/*.cpp menus/*.cpp \
              menus/dynamic/*.cpp model/*.cpp controls/*.cpp))
MENU_INC := $(addprefix -I$(MENU)/,. miniutl font controls menus model sdk_includes/common \
              sdk_includes/engine sdk_includes/public sdk_includes/pm_shared)

HLW      := crossbow crowbar egon gauss glock handgrenade hornetgun mp5 python rpg satchel shotgun \
            squeakgrenade tripmine
CLIENT_CXX := $(filter-out 3rdparty/hlsdk-portable/cl_dll/GameStudioModelRenderer_Sample.cpp, \
                $(call src,3rdparty/hlsdk-portable/cl_dll/*.cpp 3rdparty/hlsdk-portable/cl_dll/*/*.cpp)) \
              $(filter-out %/voice_vgui_tweakdlg.cpp %/voice_gamemgr.cpp %/voice_status.cpp, \
                $(call src,3rdparty/hlsdk-portable/game_shared/*.cpp)) \
              $(patsubst %,3rdparty/hlsdk-portable/dlls/%.cpp,$(HLW)) \
              $(call src,$(addprefix 3rdparty/hlsdk-portable/freevgui/,*.cpp controls/*.cpp \
                platform/common/*.cpp platform/posix/*.cpp platform/xash3d-fwgs/*.cpp))
CLIENT_C   := $(call src,3rdparty/hlsdk-portable/pm_shared/*.c) \
              3rdparty/hlsdk-portable/public/safe_snprintf.c 3rdparty/hlsdk-portable/game_shared/vcs_info.c
CLIENT_INC := $(addprefix -I$(HL)/,cl_dll dlls common engine pm_shared game_shared public \
                utils/vgui/include freevgui engine) -I$(X)/engine
SERVER_CXX := $(filter-out %/mpstubb.cpp %/stats.cpp %/Wxdebug.cpp, \
                $(call src,3rdparty/hlsdk-portable/dlls/*.cpp 3rdparty/hlsdk-portable/dlls/*/*.cpp)) \
              3rdparty/hlsdk-portable/game_shared/voice_gamemgr.cpp
SERVER_C   := $(CLIENT_C)
SERVER_INC := $(addprefix -I$(HL)/,dlls common engine pm_shared game_shared public)
# hlsdk-portable: its exports are default visibility (EXPORT), the rest hidden
HL_FLAGS   := -fvisibility=hidden -DXASH_64BIT=0 -DHAVE_TGMATH_H=1 -DHAVE_CMATH=1 \
              -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf \
              -D_vsnprintf=vsnprintf -D_LINUX=1 -DLINUX=1

obj = $(patsubst %,$(OUT)/$(1)/%.o,$(basename $(2)))

ENGINE_OBJS := $(call obj,engine,$(ENGINE_C) $(PUBLIC_C) $(BZIP2_C) $(MEMU_C)) \
               $(patsubst %,$(OUT)/zlib/%.o,$(ZLIB_C)) $(OUT)/ps2/ps2_posix.o $(OUT)/ps2/ps2_stubs.o
FS_OBJS     := $(call obj,fs,$(FS_C) $(PUBLIC_C)) $(patsubst %,$(OUT)/zlib/%.o,$(ZLIB_C))
REF_OBJS    := $(call obj,ref,$(REF_C) $(PUBLIC_C))
MENU_OBJS   := $(call obj,menu,$(MENU_CXX))
CLIENT_OBJS := $(call obj,client,$(CLIENT_CXX) $(CLIENT_C))
SERVER_OBJS := $(call obj,server,$(SERVER_CXX) $(SERVER_C))

# ---- the submodules' PS2 patches -------------------------------------------------------
PATCHED := $(OUT)/gen/patched
$(PATCHED): $(wildcard $(HERE)/ps2/patches/*.patch) $(HERE)/ps2/patches.sh
	@mkdir -p $(dir $@)
	$(HERE)/ps2/patches.sh apply
	touch $@

# ---- compiling ------------------------------------------------------------------------------
CDEPS   := $(PATCHED) $(PICOLIBC_A) $(OUT)/gen/runcfg.h
CXXDEPS := $(PATCHED) $(LIBCXX_A) $(OUT)/gen/runcfg.h

# bzip2's version (3rdparty/bzip2/wscript)
$(OUT)/gen/bz_version.h: $(X)/3rdparty/bzip2/bzip2/bz_version.h.in
	@mkdir -p $(dir $@)
	sed 's/@BZ_VERSION@/1.1.0-fwgs/' $< > $@
$(call obj,engine,$(BZIP2_C)): $(OUT)/gen/bz_version.h

$(OUT)/engine/%.o: $(X)/%.c $(CDEPS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_C) -DENGINE_DLL=1 -DXASH_ENABLE_MAIN=1 $(ENGINE_INC) \
	  -DXASH_BUILD_COMMIT=\"ps2\" -DXASH_BUILD_BRANCH=\"ps2\" -DXASH_BUILD_COMMIT_DATE=\"2026\" \
	  -D_GNU_SOURCE -DBZ_NO_STDIO=1 -DBZ_UNIX=1 -c $< -o $@
# the modules: their exports default visibility (EXPORT), the rest hidden
$(OUT)/fs/%.o: $(X)/%.c $(CDEPS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_C) -fvisibility=hidden -I$(X)/filesystem $(SDKINC) -I$(ZLIB_SRC) -c $< -o $@
$(OUT)/fs/%.o: $(X)/%.cpp $(CXXDEPS)
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS_CXX) -fvisibility=hidden -I$(X)/filesystem $(SDKINC) -I$(ZLIB_SRC) -c $< -o $@
$(OUT)/ref/%.o: $(X)/%.c $(CDEPS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_C) -fvisibility=hidden -DREF_DLL=1 -DXASH_GL_STATIC=1 $(REF_INC) -c $< -o $@
$(OUT)/menu/%.o: $(X)/%.cpp $(CXXDEPS)
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS_CXX) -fvisibility=hidden -DMAINUI_USE_CUSTOM_FONT_RENDER=1 -DMAINUI_USE_STB=1 '-DSTDINT_H=<cstdint>' $(MENU_INC) -c $< -o $@
$(OUT)/client/%.o: $(X)/%.cpp $(CXXDEPS)
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS_CXX) $(HL_FLAGS) -DCLIENT_DLL=1 -DINTERNAL_VGUI_SUPPORT=1 $(CLIENT_INC) -c $< -o $@
$(OUT)/client/%.o: $(X)/%.c $(CDEPS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_C) $(HL_FLAGS) -DCLIENT_DLL=1 $(CLIENT_INC) -c $< -o $@
$(OUT)/server/%.o: $(X)/%.cpp $(CXXDEPS)
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS_CXX) $(HL_FLAGS) $(SERVER_INC) -c $< -o $@
$(OUT)/server/%.o: $(X)/%.c $(CDEPS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_C) $(HL_FLAGS) $(SERVER_INC) -c $< -o $@
$(OUT)/zlib/%.o: $(ZLIB_SRC)/%.c $(PICOLIBC_A)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_C) -c $< -o $@
$(OUT)/ps2/%.o: $(HERE)/ps2/%.c $(PICOLIBC_A)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_C) -I$(MAXSDK)/include -c $< -o $@

# maxsdk's libc glue with its read, write and _exit renamed: ps2/ps2_posix.c
# has them (files; the end marker), and passes the console's on
$(OUT)/libc/glue.o: $(MAXSDK)/libc/glue.c $(PICOLIBC_A) $(HERE)/Makefile
	@mkdir -p $(dir $@)
	$(CC) $(filter-out -ffreestanding,$(CFLAGS)) -nostdlibinc $(SYSINC) -Dread=mx_glue_read \
	  -Dwrite=mx_glue_write -D_exit=mx_glue_exit -c $< -o $@

# ---- the modules: one relocatable object each, its export table global -------------
# The exports: the module's global, default visibility, unmangled symbols
# (built with -fvisibility=hidden: what is EXPORT, its API, and for the
# server its entities, LINK_ENTITY_TO_CLASS)
MODULES := filesystem_stdio ref_gl menu client server
filesystem_stdio_OBJS := $(FS_OBJS)
ref_gl_OBJS := $(REF_OBJS)
menu_OBJS := $(MENU_OBJS)
client_OBJS := $(CLIENT_OBJS)
server_OBJS := $(SERVER_OBJS)

define MOD_RULE
$(OUT)/mod/$(1).r.o: $$($(1)_OBJS)
	@mkdir -p $$(dir $$@)
	$$(LD_R) -o $$@ $$^
endef
$(foreach m,$(MODULES),$(eval $(call MOD_RULE,$(m))))
$(OUT)/mod/%.exports: $(OUT)/mod/%.r.o
	$(LLVM)/llvm-readelf -sW $< | awk '$$5 == "GLOBAL" && $$6 == "DEFAULT" && $$7 != "UND" && \
	  $$8 !~ /^_Z/ && $$8 != "" { print $$8 }' | sort -u > $@
$(OUT)/mod/%_exports.c: $(OUT)/mod/%.exports
	{ while read e; do [ -n "$$e" ] && echo "extern void $$e(void);"; done < $<; \
	  echo "struct { const char *name; void *func; } lib_$*_exports[] = {"; \
	  while read e; do [ -n "$$e" ] && echo "{ \"$$e\", (void *)&$$e },"; done < $<; \
	  echo "{ 0, 0 } };"; } > $@
$(OUT)/mod/%_exports.o: $(OUT)/mod/%_exports.c
	$(CC) $(CFLAGS_C) -c $< -o $@
$(OUT)/mod/%.o: $(OUT)/mod/%.r.o $(OUT)/mod/%_exports.o
	$(LD_R) -o $@.u $^
	$(OBJCOPY) -G lib_$*_exports $@.u $@
	rm -f $@.u

$(OUT)/gen/generated_library_tables.h: $(HERE)/Makefile
	@mkdir -p $(dir $@)
	{ for m in $(MODULES); do echo "extern table_t lib_$${m}_exports[];"; done; \
	  echo "struct { const char *name; void *func; } libs[] = {"; \
	  for m in $(MODULES); do echo "{ \"$$m\", &lib_$${m}_exports },"; done; \
	  echo "{ 0, 0 } };"; } > $@
$(OUT)/engine/engine/platform/misc/lib_static.o: $(OUT)/gen/generated_library_tables.h

# FRAMES: end after that many frames with a picture (host:halflife.tga);
# ARGS: more of the command line (e.g. "+map hldemo1"); DIR: the base
# directory on the file server. In $(OUT)/gen/runcfg.h, rewritten only when
# they change.
RUNCFG := $(OUT)/gen/runcfg.h
$(shell mkdir -p $(OUT)/gen; printf '#define PS2_FRAMES %s\n#define PS2_ARGS "%s"\n#define PS2_DIR "%s"\n#define PS2_VM %s\n' \
  '$(or $(FRAMES),0)' '$(ARGS)' '$(or $(DIR),/hl)' '$(if $(filter vm,$(MX_MEMORY)),1,0)' > $(RUNCFG).new; \
  cmp -s $(RUNCFG).new $(RUNCFG) || mv $(RUNCFG).new $(RUNCFG); rm -f $(RUNCFG).new)

MOD_OBJS := $(patsubst %,$(OUT)/mod/%.o,$(MODULES))
$(OUT)/halflife.elf: $(ENGINE_OBJS) $(MOD_OBJS) $(MXGL_OBJS) $(HLSL_OBJS) $(MX_LIB) $(MX_CRT0) $(MX_LD) $(LIBC_LIBS)
	$(call link,$@,--error-limit=0 --defsym=__stack_size=0x100000 $(MX_CRT0) $(ENGINE_OBJS) $(MOD_OBJS) \
	  $(MXGL_OBJS) $(HLSL_OBJS) --start-group $(LIBC_LIBS) --end-group)

# the submodules the port uses, not all of Xash3D's
submodules:
	git -C $(X) submodule update --init 3rdparty/mainui 3rdparty/hlsdk-portable \
	  3rdparty/library_suffix 3rdparty/MultiEmulator 3rdparty/bzip2/bzip2 3rdparty/extras/xash-extras
	git -C $(MENU) submodule update --init
	git -C $(HL) submodule update --init freevgui

# ---- picolibc and libc++ (as apps/descent3) -----------------------------------------
cxxlib: $(LIBCXX_A)
$(LIBCXX)/lib/libc++abi.a: $(LIBCXX_A) ;
$(MXGL_OBJS): $(PICOLIBC_A)

$(PICOLIBC_A):
	@mkdir -p $(LIBOUT)/picolibc
	sed "s|@LLVM@|$(LLVM)|g" $(MAXSDK)/libc/cross-r5900.txt.in > $(LIBOUT)/picolibc/cross.txt
	meson setup --wipe $(LIBOUT)/picolibc/build-meson $(PICOLIBC_SRC) \
	  --cross-file $(LIBOUT)/picolibc/cross.txt --prefix=$(PICOLIBC) -Dmultilib=false \
	  -Dpicocrt=false -Dsemihost=false -Dtests=false -Dthread-local-storage=false \
	  -Dposix-io=true -Dspecsdir=none -Dio-long-long=true -Dbuildtype=release
	ninja -C $(LIBOUT)/picolibc/build-meson install

$(LIBCXX_A): $(PICOLIBC_A)
	cmake -G Ninja -S $(LLVM_SRC)/runtimes -B $(LIBOUT)/libcxx/build \
	  -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" -DCMAKE_BUILD_TYPE=Release \
	  -DCMAKE_INSTALL_PREFIX=$(LIBCXX) -DCMAKE_C_COMPILER=$(LLVM)/clang \
	  -DCMAKE_CXX_COMPILER=$(LLVM)/clang++ -DCMAKE_AR=$(LLVM)/llvm-ar \
	  -DCMAKE_RANLIB=$(LLVM)/llvm-ranlib -DCMAKE_C_FLAGS="$(LIB_FLAGS)" \
	  -DCMAKE_CXX_FLAGS="$(LIB_FLAGS)" -DCMAKE_SYSTEM_NAME=Generic \
	  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY -DRUNTIMES_USE_LIBC=picolibc \
	  -DLIBCXXABI_BAREMETAL=ON -DLIBCXXABI_ENABLE_ASSERTIONS=OFF \
	  -DLIBCXXABI_ENABLE_EXCEPTIONS=OFF -DLIBCXXABI_ENABLE_SHARED=OFF \
	  -DLIBCXXABI_ENABLE_THREADS=OFF -DLIBCXXABI_USE_LLVM_UNWINDER=OFF \
	  -DLIBCXXABI_USE_COMPILER_RT=ON -DLIBCXXABI_INCLUDE_TESTS=OFF \
	  -DLIBCXX_ENABLE_EXCEPTIONS=OFF -DLIBCXX_ENABLE_FILESYSTEM=ON \
	  -DLIBCXX_ENABLE_MONOTONIC_CLOCK=ON -DLIBCXX_ENABLE_RANDOM_DEVICE=OFF \
	  -DLIBCXX_ENABLE_RTTI=ON -DLIBCXX_ENABLE_SHARED=OFF -DLIBCXX_ENABLE_THREADS=OFF \
	  -DLIBCXX_ENABLE_WIDE_CHARACTERS=OFF -DLIBCXX_ENABLE_LOCALIZATION=ON \
	  -DLIBCXX_ENABLE_TIME_ZONE_DATABASE=OFF -DLIBCXX_INCLUDE_BENCHMARKS=OFF \
	  -DLIBCXX_INCLUDE_TESTS=OFF -DLIBCXX_USE_COMPILER_RT=ON -DLIBCXX_CXX_ABI=libcxxabi \
	  -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON -DLIBCXX_SHARED_OUTPUT_NAME=c++-shared \
	  -DLIBCXXABI_SHARED_OUTPUT_NAME=c++abi-shared
	ninja -C $(LIBOUT)/libcxx/build install-cxx install-cxxabi

# ---- running --------------------------------------------------------------------
# the Uplink demo's valve directory (pak0.pak, gfx.wad, decals.wad, ...)
HLDATA   ?= $(PS2MAX)/build/data/hluplink/valve
RUNDIR   := $(PS2MAX)/build/pcsx2-test
# Xash3D's extras (fonts, gamepad presets), as 3rdparty/extras/wscript makes them
$(OUT)/extras.pk3: $(wildcard $(X)/3rdparty/extras/xash-extras/*)
	cd $(X)/3rdparty/extras/xash-extras && python3 -c "import os, zipfile; z = zipfile.ZipFile('$@', 'w', zipfile.ZIP_DEFLATED); [z.write(os.path.join(d, f)) for d, s, fs in os.walk('.') for f in fs if not d.startswith('./scripts') and not f.startswith('.') and not f.endswith('.svg')]"
run: $(OUT)/halflife.elf $(OUT)/extras.pk3
	@[ -f $(RUNDIR)/hl/valve/pak0.pak ] || { mkdir -p $(RUNDIR)/hl/valve && cp -rv "$(HLDATA)"/. $(RUNDIR)/hl/valve/; }
	cp $(HERE)/ps2/delta.lst $(OUT)/extras.pk3 $(RUNDIR)/hl/valve/
ifeq ($(MX_MEMORY),vm)
	cp $(OUT)/halflife.elf $(RUNDIR)/halflife.elf && echo "host:halflife.elf" > $(RUNDIR)/mxload.txt
	PCSX2_EE=interp $(PCSX2_RUN) $(MX_LOADER) $(or $(TIMEOUT),3600)
else
	$(PCSX2_RUN) $(OUT)/halflife.elf $(or $(TIMEOUT),600)
endif

clean:
	rm -rf $(OUT)/engine $(OUT)/fs $(OUT)/ref $(OUT)/menu $(OUT)/client $(OUT)/server \
	  $(OUT)/mod $(OUT)/zlib $(OUT)/ps2 $(OUT)/libc $(OUT)/gen $(OUT)/*.elf $(OUT)/*.map

.PHONY: all submodules cxxlib run clean

-include $(shell find $(OUT) -name '*.d' 2>/dev/null)
