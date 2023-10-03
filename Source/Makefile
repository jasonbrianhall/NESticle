# Compiler settings
CXX = g++
CXXFLAGS = -c -Wall

# List of .cpp files (add all your .cpp files here)
SOURCES = build.cpp command.cpp config.cpp cpu.cpp disasm.cpp file.cpp font.cpp input.cpp inputw.cpp main.cpp message.cpp mmc.cpp mouse.cpp nes.cpp nesdlg.cpp nessound.cpp nesvideo.cpp prof.cpp r2img.cpp rom.cpp slist.cpp snapshot.cpp stddlg.cpp vol.cpp

# List of object files (replace .cpp with .o) 
OBJECTS = $(SOURCES:.cpp=.o) 

# Default target
all: $(OBJECTS)

# Compile rule 
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# Clean rule
clean:
	rm -f $(OBJECTS)
