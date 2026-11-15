# NESticle for Windows (64-bit), cross-compiled with MinGW-w64 and SDL3
#
#   make            nesticle.exe (+ SDL3.dll next to it)
#   make dist       build/dist/NESticle: nesticle.exe, SDL3.dll, GUI.VOL, README
#   make clean      (distclean also removes the downloaded SDL3)
#
# Needs MinGW-w64 and make:
#   Fedora:         sudo dnf install mingw64-gcc-c++ make
#   Debian/Ubuntu:  sudo apt install g++-mingw-w64-x86-64 make
#   Windows (MSYS2): pacman -S mingw-w64-x86_64-gcc make, then make CROSS=
# SDL3's MinGW development package is downloaded from libsdl-org/SDL's
# releases the first time (or point SDL3_DIR at an unpacked one).
#
# Same sources as the DOS build: ../Source, ../Source/port, and the SDL3
# platform layer ../msdos/sdlmain.cpp (in a window here).

SRC   := ../Source
PORT  := ../Source/port
PLAT  := ../msdos
BUILD := build

CROSS ?= x86_64-w64-mingw32-
CXX   := $(CROSS)g++
ARCH  := x86_64-w64-mingw32

SDL3_VERSION ?= 3.4.20
SDL3_URL     := https://github.com/libsdl-org/SDL/releases/download/release-$(SDL3_VERSION)/SDL3-devel-$(SDL3_VERSION)-mingw.tar.gz
SDL3_DIR     ?= $(BUILD)/SDL3-$(SDL3_VERSION)/$(ARCH)
SDL3_STAMP   := $(SDL3_DIR)/include/SDL3/SDL.h

# -funsigned-char: the code expects Watcom's unsigned char
CXXFLAGS ?= -O2
CXXFLAGS += -fpermissive -funsigned-char -fno-strict-aliasing -fno-delete-null-pointer-checks \
            -fno-exceptions -w -DDOS -DSDL_PORT -include $(PORT)/port.h -I$(SRC) -I$(SDL3_DIR)/include
# -mwindows: no console window; static libgcc/libstdc++: only SDL3.dll to ship
LDFLAGS  = -mwindows -static-libgcc -static-libstdc++ -L$(SDL3_DIR)/lib -lSDL3 -lm

NES_SRCS = build.cpp command.cpp config.cpp cpu.cpp disasm.cpp file.cpp font.cpp input.cpp \
           inputw.cpp main.cpp message.cpp mmc.cpp mouse.cpp nes.cpp nesdlg.cpp nessound.cpp \
           nesvideo.cpp prof.cpp r2img.cpp rom.cpp slist.cpp snapshot.cpp stddlg.cpp vol.cpp
PORT_SRCS = m6502.cpp draw.cpp guirect.cpp gui.cpp guimenu.cpp nesapu.cpp guidata.cpp nespal.cpp

OBJS = $(addprefix $(BUILD)/obj/,$(NES_SRCS:.cpp=.o)) \
       $(addprefix $(BUILD)/obj/port/,$(PORT_SRCS:.cpp=.o)) \
       $(BUILD)/obj/sdlmain.o
HDRS = $(wildcard $(SRC)/*.h) $(wildcard $(PORT)/*.h)

TARGET = nesticle.exe

all: $(TARGET)

$(SDL3_STAMP):
	@mkdir -p $(BUILD)
	@echo "Getting SDL3 $(SDL3_VERSION) for MinGW"
	curl -sSL -o $(BUILD)/sdl3-mingw.tar.gz $(SDL3_URL)
	tar xzf $(BUILD)/sdl3-mingw.tar.gz -C $(BUILD)
	rm -f $(BUILD)/sdl3-mingw.tar.gz
	@touch $@

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	cp $(SDL3_DIR)/bin/SDL3.dll .

$(BUILD)/obj/%.o: $(SRC)/%.cpp $(HDRS) | $(SDL3_STAMP)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/obj/port/%.o: $(PORT)/%.cpp $(HDRS) | $(SDL3_STAMP)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/obj/sdlmain.o: $(PLAT)/sdlmain.cpp $(HDRS) | $(SDL3_STAMP)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

dist: $(TARGET)
	rm -rf $(BUILD)/dist && mkdir -p $(BUILD)/dist/NESticle
	cp $(TARGET) SDL3.dll $(BUILD)/dist/NESticle/
	sed 's/$$/\r/' README.md > $(BUILD)/dist/NESticle/README.txt
	[ ! -f $(PLAT)/GUI.VOL ] || cp $(PLAT)/GUI.VOL $(BUILD)/dist/NESticle/
	@echo "Created $(BUILD)/dist/NESticle"

clean:
	rm -rf $(BUILD)/obj $(BUILD)/dist $(TARGET) SDL3.dll

distclean:
	rm -rf $(BUILD) $(TARGET) SDL3.dll

.PHONY: all dist clean distclean
