# NESticle for MS-DOS / FreeDOS (DJGPP, 32-bit DPMI)
#
#   make            nesticle.exe with CWSDPMI embedded (runs on plain DOS)
#   make msdos_original
#                   nesticle.exe that needs CWSDPMI.EXE next to it (or any
#                   other DPMI host: Windows 9x, HDPMI32, ...)
#   make dist       build/dist/NESTICLE: the program folder, ready for a DOS disk
#   make run        run it in DOSBox
#   make clean
#
# Needs a DJGPP cross compiler on the PATH (i586-pc-msdosdjgpp-gcc / g++;
# prebuilt: https://github.com/andrewwutw/build-djgpp/releases), or Docker,
# which gets the djfdyuruiry/djgpp image instead. Plus make.
#
#   SDL3/         SDL3 source with its DOS port (VESA/VGA video, Sound Blaster,
#                 keyboard, INT 33h mouse, gameport), compiled here into
#                 build/libSDL3.a; no CMake. (Same tree as SDLPoPDOS/msdos.)
#   sdl3-config/  the two headers SDL's CMake would generate, for DJGPP.
#   cwsdpmi/      CWSDPMI r7 (the DPMI host; CWSDSTUB.EXE goes in front of the EXE).
#
# The emulator is the original NESticle source (../Source), with the pieces
# that were never released (GUI library, x86 TASM cores) rewritten in C++ in
# ../Source/port, and the platform layer (video, keyboard, mouse, timer,
# sound) in sdlmain.cpp on top of SDL3.

SRC     := ../Source
PORT    := ../Source/port
SDL     := SDL3
BUILD   := build

DOS_TARGET      = nesticle.exe
COFF_TARGET     = $(BUILD)/nesticle.coff
SDL3_LIB        = $(BUILD)/libSDL3.a

DJGPP_IMAGE = djfdyuruiry/djgpp

USER_ID  = $(shell id -u)
GROUP_ID = $(shell id -g)

CROSS ?= i586-pc-msdosdjgpp-
ifeq ($(origin DOCKER),undefined)
DOCKER := $(if $(shell command -v $(CROSS)gcc 2>/dev/null),0,1)
endif

ifeq ($(DOCKER),1)
RUN      = docker run --rm -v $(abspath ..):/src:z -w /src/msdos -u $(USER_ID):$(GROUP_ID) $(DJGPP_IMAGE) /bin/sh -c
CC_DOS   = gcc
CXX_DOS  = g++
AR_DOS   = ar
EXE2COFF = exe2coff
else
CC_DOS   = $(CROSS)gcc
CXX_DOS  = $(CROSS)g++
AR_DOS   = $(CROSS)ar
EXE2COFF = $(shell command -v $(CROSS)exe2coff 2>/dev/null || echo $(dir $(shell command -v $(CROSS)gcc))../i586-pc-msdosdjgpp/bin/exe2coff)
endif

all: msdos_embedded

# SDL3, sdl3-config and cwsdpmi are the same as in SDLPoPDOS/msdos: if they
# aren't here, get them from there first (on the host, before any Docker
# run, and before the SDL3 file list below is read).
SDLPOPDOS_REPO = https://github.com/jasonbrianhall/SDLPoPDOS
ifeq ($(filter clean distclean,$(MAKECMDGOALS)),)
NEED_DEPS := $(if $(and $(wildcard SDL3/include/SDL3/SDL.h),$(wildcard sdl3-config/build_config),$(wildcard cwsdpmi/bin/CWSDSTUB.EXE)),,1)
ifeq ($(NEED_DEPS),1)
$(info Getting SDL3, sdl3-config and cwsdpmi from $(SDLPOPDOS_REPO) ...)
DEPS_RESULT := $(shell rm -rf $(BUILD)/SDLPoPDOS && mkdir -p $(BUILD) && \
  git clone -q --depth 1 $(SDLPOPDOS_REPO) $(BUILD)/SDLPoPDOS && \
  rm -rf SDL3 sdl3-config cwsdpmi && \
  cp -r $(BUILD)/SDLPoPDOS/msdos/SDL3 $(BUILD)/SDLPoPDOS/msdos/sdl3-config $(BUILD)/SDLPoPDOS/msdos/cwsdpmi . && \
  rm -rf $(BUILD)/SDLPoPDOS && echo ok)
ifneq ($(DEPS_RESULT),ok)
$(error Couldn't get them; copy SDL3, sdl3-config and cwsdpmi from SDLPoPDOS/msdos into this folder)
endif
endif
endif

# ---------------------------------------------------------------- SDL3
SDL_CFLAGS = -O2 -DNDEBUG -fno-strict-aliasing -w \
             -DSDL_BUILD_MAJOR_VERSION=3 -DSDL_BUILD_MINOR_VERSION=5 -DSDL_BUILD_MICRO_VERSION=0 \
             -DSDL_STATIC_LIB -DUSING_GENERATED_CONFIG_H \
             -Isdl3-config/build_config -Isdl3-config -I$(SDL)/include -I$(SDL)/src \
             -idirafter $(SDL)/src/video/khronos

# The SDL3 sources for DJGPP (what SDL's own build compiles for this target).
SDL_SRCS = \
  src/SDL.c \
  src/SDL_assert.c \
  src/SDL_error.c \
  src/SDL_guid.c \
  src/SDL_hashtable.c \
  src/SDL_hints.c \
  src/SDL_list.c \
  src/SDL_log.c \
  src/SDL_properties.c \
  src/SDL_utils.c \
  src/atomic/SDL_atomic.c \
  src/atomic/SDL_spinlock.c \
  src/audio/SDL_audio.c \
  src/audio/SDL_audiocvt.c \
  src/audio/SDL_audiodev.c \
  src/audio/SDL_audioqueue.c \
  src/audio/SDL_audioresample.c \
  src/audio/SDL_audiotypecvt.c \
  src/audio/SDL_mixer.c \
  src/audio/SDL_wave.c \
  src/audio/disk/SDL_diskaudio.c \
  src/audio/dos/SDL_dosaudio_sb.c \
  src/audio/dummy/SDL_dummyaudio.c \
  src/camera/SDL_camera.c \
  src/camera/dummy/SDL_camera_dummy.c \
  src/core/SDL_core_unsupported.c \
  src/core/dos/SDL_dos.c \
  src/core/dos/SDL_dos_scheduler.c \
  src/cpuinfo/SDL_cpuinfo.c \
  src/dialog/SDL_dialog.c \
  src/dialog/dummy/SDL_dummydialog.c \
  src/dynapi/SDL_dynapi.c \
  src/events/SDL_categories.c \
  src/events/SDL_clipboardevents.c \
  src/events/SDL_displayevents.c \
  src/events/SDL_dropevents.c \
  src/events/SDL_events.c \
  src/events/SDL_eventwatch.c \
  src/events/SDL_keyboard.c \
  src/events/SDL_keymap.c \
  src/events/SDL_keysym_to_keycode.c \
  src/events/SDL_keysym_to_scancode.c \
  src/events/SDL_mouse.c \
  src/events/SDL_notificationevents.c \
  src/events/SDL_pen.c \
  src/events/SDL_quit.c \
  src/events/SDL_scancode_tables.c \
  src/events/SDL_touch.c \
  src/events/SDL_windowevents.c \
  src/events/imKStoUCS.c \
  src/filesystem/SDL_filesystem.c \
  src/filesystem/dos/SDL_sysfilesystem.c \
  src/filesystem/posix/SDL_sysfsops.c \
  src/gpu/SDL_gpu.c \
  src/gpu/xr/SDL_gpu_openxr.c \
  src/gpu/xr/SDL_openxrdyn.c \
  src/haptic/SDL_haptic.c \
  src/haptic/dummy/SDL_syshaptic.c \
  src/hidapi/SDL_hidapi.c \
  src/io/SDL_asyncio.c \
  src/io/SDL_iostream.c \
  src/io/generic/SDL_asyncio_generic.c \
  src/joystick/SDL_gamepad.c \
  src/joystick/SDL_joystick.c \
  src/joystick/SDL_steam_virtual_gamepad.c \
  src/joystick/controller_type.c \
  src/joystick/dos/SDL_sysjoystick.c \
  src/joystick/virtual/SDL_virtualjoystick.c \
  src/libm/e_atan2.c \
  src/libm/e_exp.c \
  src/libm/e_fmod.c \
  src/libm/e_log.c \
  src/libm/e_log10.c \
  src/libm/e_pow.c \
  src/libm/e_rem_pio2.c \
  src/libm/e_sqrt.c \
  src/libm/k_cos.c \
  src/libm/k_rem_pio2.c \
  src/libm/k_sin.c \
  src/libm/k_tan.c \
  src/libm/s_atan.c \
  src/libm/s_copysign.c \
  src/libm/s_cos.c \
  src/libm/s_fabs.c \
  src/libm/s_floor.c \
  src/libm/s_isinf.c \
  src/libm/s_isinff.c \
  src/libm/s_isnan.c \
  src/libm/s_isnanf.c \
  src/libm/s_modf.c \
  src/libm/s_scalbn.c \
  src/libm/s_sin.c \
  src/libm/s_tan.c \
  src/loadso/dummy/SDL_sysloadso.c \
  src/locale/SDL_locale.c \
  src/locale/dummy/SDL_syslocale.c \
  src/main/SDL_main_callbacks.c \
  src/main/SDL_runapp.c \
  src/main/dos/SDL_sysmain_runapp.c \
  src/main/generic/SDL_sysmain_callbacks.c \
  src/misc/SDL_libusb.c \
  src/misc/SDL_url.c \
  src/misc/dummy/SDL_sysurl.c \
  src/notification/SDL_notification.c \
  src/notification/dummy/SDL_dummynotification.c \
  src/power/SDL_power.c \
  src/process/SDL_process.c \
  src/process/dummy/SDL_dummyprocess.c \
  src/render/SDL_render.c \
  src/render/SDL_render_unsupported.c \
  src/render/SDL_yuv_sw.c \
  src/render/direct3d/SDL_render_d3d.c \
  src/render/direct3d/SDL_shaders_d3d.c \
  src/render/direct3d11/SDL_render_d3d11.c \
  src/render/direct3d11/SDL_shaders_d3d11.c \
  src/render/direct3d12/SDL_render_d3d12.c \
  src/render/direct3d12/SDL_shaders_d3d12.c \
  src/render/gpu/SDL_pipeline_gpu.c \
  src/render/gpu/SDL_render_gpu.c \
  src/render/gpu/SDL_shaders_gpu.c \
  src/render/ngage/SDL_render_ngage.c \
  src/render/opengl/SDL_render_gl.c \
  src/render/opengl/SDL_shaders_gl.c \
  src/render/opengles/SDL_render_gles.c \
  src/render/opengles2/SDL_render_gles2.c \
  src/render/opengles2/SDL_shaders_gles2.c \
  src/render/ps2/SDL_render_ps2.c \
  src/render/psp/SDL_render_psp.c \
  src/render/software/SDL_blendfillrect.c \
  src/render/software/SDL_blendline.c \
  src/render/software/SDL_blendpoint.c \
  src/render/software/SDL_drawline.c \
  src/render/software/SDL_drawpoint.c \
  src/render/software/SDL_render_sw.c \
  src/render/software/SDL_triangle.c \
  src/render/vitagxm/SDL_render_vita_gxm.c \
  src/render/vitagxm/SDL_render_vita_gxm_memory.c \
  src/render/vitagxm/SDL_render_vita_gxm_tools.c \
  src/render/vulkan/SDL_render_vulkan.c \
  src/render/vulkan/SDL_shaders_vulkan.c \
  src/sensor/SDL_sensor.c \
  src/sensor/dummy/SDL_dummysensor.c \
  src/stdlib/SDL_crc16.c \
  src/stdlib/SDL_crc32.c \
  src/stdlib/SDL_getenv.c \
  src/stdlib/SDL_iconv.c \
  src/stdlib/SDL_malloc.c \
  src/stdlib/SDL_memcpy.c \
  src/stdlib/SDL_memmove.c \
  src/stdlib/SDL_memset.c \
  src/stdlib/SDL_mslibc.c \
  src/stdlib/SDL_murmur3.c \
  src/stdlib/SDL_qsort.c \
  src/stdlib/SDL_random.c \
  src/stdlib/SDL_stdlib.c \
  src/stdlib/SDL_string.c \
  src/stdlib/SDL_strtokr.c \
  src/storage/SDL_storage.c \
  src/storage/generic/SDL_genericstorage.c \
  src/thread/SDL_thread.c \
  src/thread/dos/SDL_sysmutex.c \
  src/thread/dos/SDL_syssem.c \
  src/thread/dos/SDL_systhread.c \
  src/thread/dos/SDL_systls.c \
  src/thread/generic/SDL_syscond.c \
  src/thread/generic/SDL_sysrwlock.c \
  src/time/SDL_time.c \
  src/time/unix/SDL_systime.c \
  src/timer/SDL_timer.c \
  src/timer/dos/SDL_systimer.c \
  src/tray/SDL_tray_utils.c \
  src/tray/dummy/SDL_tray.c \
  src/video/SDL_RLEaccel.c \
  src/video/SDL_blit.c \
  src/video/SDL_blit_0.c \
  src/video/SDL_blit_1.c \
  src/video/SDL_blit_A.c \
  src/video/SDL_blit_N.c \
  src/video/SDL_blit_auto.c \
  src/video/SDL_blit_copy.c \
  src/video/SDL_blit_slow.c \
  src/video/SDL_bmp.c \
  src/video/SDL_clipboard.c \
  src/video/SDL_egl.c \
  src/video/SDL_fillrect.c \
  src/video/SDL_pixels.c \
  src/video/SDL_rect.c \
  src/video/SDL_rotate.c \
  src/video/SDL_stb.c \
  src/video/SDL_stretch.c \
  src/video/SDL_surface.c \
  src/video/SDL_video.c \
  src/video/SDL_video_unsupported.c \
  src/video/SDL_vulkan_utils.c \
  src/video/SDL_yuv.c \
  src/video/dos/SDL_dosevents.c \
  src/video/dos/SDL_dosframebuffer.c \
  src/video/dos/SDL_dosmodes.c \
  src/video/dos/SDL_dosmouse.c \
  src/video/dos/SDL_dosvideo.c \
  src/video/dummy/SDL_nullevents.c \
  src/video/dummy/SDL_nullframebuffer.c \
  src/video/dummy/SDL_nullvideo.c \
  src/video/yuv2rgb/yuv_rgb_lsx.c \
  src/video/yuv2rgb/yuv_rgb_sse.c \
  src/video/yuv2rgb/yuv_rgb_std.c

SDL_OBJS = $(addprefix $(BUILD)/sdl/,$(SDL_SRCS:.c=.o))

$(BUILD)/sdl/%.o: $(SDL)/%.c
	@mkdir -p $(dir $@)
	$(CC_DOS) $(SDL_CFLAGS) -MMD -MP -c $< -o $@

-include $(SDL_OBJS:.o=.d)

$(SDL3_LIB): $(SDL_OBJS)
	rm -f $@
	$(AR_DOS) rcs $@ $(SDL_OBJS)

sdl3: $(SDL3_LIB)

# ---------------------------------------------------------------- NESticle
# -funsigned-char: Watcom's char is unsigned, and the code relies on it
# (bitfields in NES_sprite, scan codes, colors).
CXXFLAGS = -O2 -march=i486 -mtune=i586 -fpermissive -funsigned-char -fno-strict-aliasing -fno-delete-null-pointer-checks \
           -fno-exceptions -w -DDOS -DSDL_PORT -include $(PORT)/port.h -I$(SRC) -I$(SDL)/include
LDFLAGS  = -s

# The original sources (ntest.cpp is a standalone CPU tester, not part of the program)
NES_SRCS = build.cpp command.cpp config.cpp cpu.cpp disasm.cpp file.cpp font.cpp input.cpp \
           inputw.cpp main.cpp message.cpp mmc.cpp mouse.cpp nes.cpp nesdlg.cpp nessound.cpp \
           nesvideo.cpp prof.cpp r2img.cpp rom.cpp slist.cpp snapshot.cpp stddlg.cpp vol.cpp
# The missing pieces, rewritten (see ../Source/port/README.md)
PORT_SRCS = m6502.cpp draw.cpp guirect.cpp gui.cpp guimenu.cpp nesapu.cpp guidata.cpp nespal.cpp
# The DOS platform layer on SDL3
PLAT_SRCS = sdlmain.cpp

OBJS = $(addprefix $(BUILD)/obj/,$(NES_SRCS:.cpp=.o)) \
       $(addprefix $(BUILD)/obj/port/,$(PORT_SRCS:.cpp=.o)) \
       $(addprefix $(BUILD)/obj/plat/,$(PLAT_SRCS:.cpp=.o))
HDRS = $(wildcard $(SRC)/*.h) $(wildcard $(PORT)/*.h)

$(BUILD)/obj/%.o: $(SRC)/%.cpp $(HDRS)
	@mkdir -p $(dir $@)
	$(CXX_DOS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/obj/port/%.o: $(PORT)/%.cpp $(HDRS)
	@mkdir -p $(dir $@)
	$(CXX_DOS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/obj/plat/%.o: %.cpp $(HDRS)
	@mkdir -p $(dir $@)
	$(CXX_DOS) $(CXXFLAGS) -c $< -o $@

ifneq ($(DOCKER),1)
$(BUILD)/nesticle-go32.exe: $(OBJS) $(SDL3_LIB)
	$(CXX_DOS) $(OBJS) $(LDFLAGS) -o $@ $(SDL3_LIB) -lm
endif

ifeq ($(DOCKER),1)
pull-djgpp:
	docker pull $(DJGPP_IMAGE)

$(BUILD)/nesticle-go32.exe $(COFF_TARGET): FORCE | pull-djgpp
	$(RUN) "make DOCKER=0 CROSS= CC_DOS=gcc CXX_DOS=g++ AR_DOS=ar EXE2COFF=exe2coff $(COFF_TARGET)"

FORCE:
.PHONY: pull-djgpp FORCE
else
$(COFF_TARGET): $(BUILD)/nesticle-go32.exe
	cp $< $(BUILD)/nesticle.exe
	cd $(BUILD) && $(EXE2COFF) nesticle.exe && mv nesticle nesticle.coff && rm nesticle.exe
endif

# Needs CWSDPMI.EXE (or another DPMI host) at run time.
msdos_original: $(BUILD)/nesticle-go32.exe
	cp $(BUILD)/nesticle-go32.exe $(DOS_TARGET)
	cp cwsdpmi/bin/CWSDPMI.EXE .

# CWSDPMI's stub in front: one file that runs on plain DOS.
msdos_embedded: $(COFF_TARGET)
	cat cwsdpmi/bin/CWSDSTUB.EXE $(COFF_TARGET) > $(DOS_TARGET)
	chmod +x $(DOS_TARGET)
	@echo "Created $(DOS_TARGET) with embedded DPMI extender"

msdos: msdos_embedded

# ---------------------------------------------------------------- packaging
dist: msdos_embedded
	rm -rf $(BUILD)/dist && mkdir -p $(BUILD)/dist/NESTICLE
	cp $(DOS_TARGET) $(BUILD)/dist/NESTICLE/NESTICLE.EXE
	for f in GUI.VOL CURSOR.BMP; do [ ! -f $$f ] || cp $$f $(BUILD)/dist/NESTICLE/; done
	sed 's/$$/\r/' README.md > $(BUILD)/dist/NESTICLE/README.TXT
	@echo "Created $(BUILD)/dist/NESTICLE"

# DOSBox with this folder as C:.
run: msdos_embedded
	printf '[cpu]\ncore=dynamic\ncputype=pentium_slow\ncycles=max\n[dosbox]\nmachine=svga_s3\nmemsize=64\n[sblaster]\nsbtype=sb16\n[autoexec]\nmount c $(abspath .)\nc:\nSET BLASTER=A220 I7 D1 H5 T6\nNESTICLE.EXE\n' > $(BUILD)/dosbox.conf
	dosbox -conf $(BUILD)/dosbox.conf

clean:
	rm -rf $(BUILD)/obj $(BUILD)/nesticle* $(DOS_TARGET) CWSDPMI.EXE

distclean:
	rm -rf $(BUILD) $(DOS_TARGET) CWSDPMI.EXE

.PHONY: all sdl3 msdos msdos_original msdos_embedded dist run clean distclean
