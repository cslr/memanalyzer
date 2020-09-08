# GNU Make Makefile for hypercube vstplugin
# (C) Copyright Tomas Ukkonen 2018
############################################################

CC = gcc
CXX = g++
MAKE = make
MKDIR = mkdir
AR = ar
CD = cd
RM = rm -f
MV = mv
CP = cp


# `pkg-config sdl2 --cflags` `pkg-config --cflags SDL2_ttf` `pkg-config --cflags SDL2_image` `pkg-config --cflags SDL2_mixer`

## -std=c++2a

OPTIMIZE=-O3 -march=native -fopenmp -ffast-math
# OPTIMIZE=

CFLAGS = -fPIC -g $(OPTIMIZE) -Wno-deprecated `pkg-config --cflags dinrhiw` `pkg-config --cflags sdl2` -I. -Wno-attributes

# -municode

CXXFLAGS = -fPIC -g $(OPTIMIZE) -Wno-deprecated  `pkg-config --cflags dinrhiw` `pkg-config --cflags sdl2`

# -municode

# VSTGUI.cpp
SOURCES=SDLGUI.cpp main.cpp


TARGET=memvisualizer
LINUXLIBS=-ldl -lpthread `pkg-config dinrhiw --libs` `pkg-config sdl2 --libs` -lstdc++fs
## WINLIBS=-static -Wl,-Bstatic -lpthread `pkg-config dinrhiw --libs` -static /mingw64/bin/libgmp-10.dll
WINLIBS=-lpthread -lgmp `pkg-config dinrhiw --libs` `pkg-config sdl2 --libs` -lstdc++fs
##LIBS=$(WINLIBS)
LIBS=$(LINUXLIBS)

OBJECTS=SDLGUI.o main.o

############################################################


all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

clean:
	$(RM) $(OBJECTS) $(TARGET)
	$(RM) *~

depend:
	$(CXX) $(CXXFLAGS) -MM $(SOURCES) > Makefile.depend

############################################################

include Makefile.depend
