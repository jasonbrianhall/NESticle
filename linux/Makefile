# NESticle for Linux (SDL3)
#
#   make            ./nesticle
#   make dist       build/dist/nesticle: the program, GUI.VOL and README
#   make clean
#
# Needs g++, make and SDL3's development files:
#   Fedora:         sudo dnf install gcc-c++ make SDL3-devel
#   Debian/Ubuntu:  sudo apt install g++ make libsdl3-dev   (Ubuntu 25.04+)
# (found with pkg-config sdl3; or set SDL3_CFLAGS / SDL3_LIBS)
#
# Same sources as the DOS build: ../Source, ../Source/port, and the SDL3
# platform layer ../msdos/sdlmain.cpp (in a window here).

SRC   := ../Source
PORT  := ../Source/port
PLAT  := ../msdos
BUILD := build

CXX ?= g++
SDL3_CFLAGS ?= $(shell pkg-config --cflags sdl3)
SDL3_LIBS   ?= $(shell pkg-config --libs sdl3)

# -funsigned-char: the code expects Watcom's unsigned char
CXXFLAGS ?= -O2
CXXFLAGS += -fpermissive -funsigned-char -fno-strict-aliasing -fno-delete-null-pointer-checks \
            -fno-exceptions -w -DDOS -DSDL_PORT -include $(PORT)/port.h -I$(SRC) $(SDL3_CFLAGS)

NES_SRCS = build.cpp command.cpp config.cpp cpu.cpp disasm.cpp file.cpp font.cpp input.cpp \
           inputw.cpp main.cpp message.cpp mmc.cpp mouse.cpp nes.cpp nesdlg.cpp nessound.cpp \
           nesvideo.cpp prof.cpp r2img.cpp rom.cpp slist.cpp snapshot.cpp stddlg.cpp vol.cpp
PORT_SRCS = m6502.cpp draw.cpp guirect.cpp gui.cpp guimenu.cpp nesapu.cpp guidata.cpp nespal.cpp guivolembed.cpp

OBJS = $(addprefix $(BUILD)/obj/,$(NES_SRCS:.cpp=.o)) \
       $(addprefix $(BUILD)/obj/port/,$(PORT_SRCS:.cpp=.o)) \
       $(BUILD)/obj/sdlmain.o
HDRS = $(wildcard $(SRC)/*.h) $(wildcard $(PORT)/*.h)

.DEFAULT_GOAL := all

# The original GUI.VOL, built into the program (port/guivolembed.cpp)
GUIVOL := $(wildcard ../msdos/GUI.VOL)
ifneq ($(GUIVOL),)
$(BUILD)/obj/port/guivolembed.o: CXXFLAGS += -DNESTICLE_GUIVOL_FILE='"$(GUIVOL)"'
$(BUILD)/obj/port/guivolembed.o: $(GUIVOL)
endif

TARGET = nesticle

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(SDL3_LIBS) -lm

$(BUILD)/obj/%.o: $(SRC)/%.cpp $(HDRS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/obj/port/%.o: $(PORT)/%.cpp $(HDRS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/obj/sdlmain.o: $(PLAT)/sdlmain.cpp $(HDRS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

dist: $(TARGET)
	rm -rf $(BUILD)/dist && mkdir -p $(BUILD)/dist/nesticle
	cp $(TARGET) README.md $(BUILD)/dist/nesticle/
	[ ! -f $(PLAT)/GUI.VOL ] || cp $(PLAT)/GUI.VOL $(BUILD)/dist/nesticle/
	@echo "Created $(BUILD)/dist/nesticle"

clean:
	rm -rf $(BUILD) $(TARGET)

.PHONY: all dist clean
