# generalised makefile for gtkmm

# name the target
PROGRAM = clock

# list the source files (as a list of a wild card)
#SRCS = clock.cpp
SRCS = $(wildcard *.cpp)

# if the include files are in a sub folder...
#INCLUDES = -I../include

# now the works

CXX = g++
CXXFLAGS = `pkg-config gtkmm-3.0 --cflags` -std=c++17 -g -Wall
OBJS = $(SRCS:.cpp=.o)
DEPDIR = .
LIBS = `pkg-config --libs gtkmm-3.0`

all: $(PROGRAM)

-include $(OBJS:%.o=$(DEPDIR)/%.Po)

%.o: %.cpp
	$(CXX) -MT $@ -MD -MP -MF $*.Tpo -c -o $@ $(CXXFLAGS) $<

$(PROGRAM): $(OBJS)
	$(CXX) -o $(PROGRAM) $(OBJS) $(LIBS)

# DO NOT DELETE THIS LINE -- make depend needs it
