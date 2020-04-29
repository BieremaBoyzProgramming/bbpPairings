
OBJECTS = main.o fileformats/generatorconfiguration.o fileformats/trf.o \
	matching/computer.o matching/detail/graph.o matching/detail/parentblossom.o \
	matching/detail/rootblossom.o swisssystems/burstein.o swisssystems/common.o \
	swisssystems/dutch.o tournament/checker.o tournament/generator.o \
	tournament/tournament.o

# Flags specifying which Swiss systems to include.
burstein = yes
dutch = yes

# Flag indicating whether the tournament checker and random tournament generator
# should be included
engine_comparison = yes

# The maximum type sizes that the build should attempt to support.
# Default values are set/computed in tournament/tournament.h based on the
# constraints imposed by the TRF(x) format and build limitations.
max_players = 0
# Points are specified in tenths. So a value of 999 means that the max total
# score of a player (with acceleration) is 99.9.
max_points = 0
max_rating = 0
max_rounds = 0

bits = 64
debug = no
profile = no
optimize = yes
static = no
COMP = gcc

CXXFLAGS += -std=c++17 -I. -m$(bits) -MD -MP -Wpedantic -pedantic-errors \
	-Wall -Wextra -Wstrict-overflow=4 -Wundef -Wshadow -Wcast-qual \
	-Wcast-align -Wmissing-declarations -Wredundant-decls -Wvla \
	-Wno-unused-parameter -Wno-sign-compare -Wno-overflow -Wno-shadow -DFILESYSTEM

VERSION_INFO="$(shell git describe --exact-match 2> /dev/null)"
ifeq ($(VERSION_INFO),"")
	VERSION_INFO="non-release build $(shell git describe 2> /dev/null)"
endif
CXXFLAGS += -DVERSION_INFO=$(VERSION_INFO)

ifeq ($(static),yes)
	LDFLAGS += -static
endif

ifeq ($(profile),yes)
	CXXFLAGS += -ggdb
	CXXFLAGS += -O3
	ifneq ($(debug),yes)
		CXXFLAGS += -DNDEBUG
	endif
else
	ifeq ($(debug),yes)
		CXXFLAGS += -ggdb
		ifeq ($(optimize),yes)
			CXXFLAGS += -Og
		endif
	else
		CXXFLAGS += -DNDEBUG
		LDFLAGS += -s
		ifeq ($(optimize),yes)
			CXXFLAGS += -O3
		endif
	endif
endif

ifeq ($(optimize),yes)
	CXXFLAGS += -flto
endif

ifneq ($(burstein),yes)
	CXXFLAGS += -DOMIT_BURSTEIN
endif
ifneq ($(dutch),yes)
	CXXFLAGS += -DOMIT_DUTCH
endif
ifneq ($(engine_comparison),yes)
	CXXFLAGS += -DOMIT_GENERATOR -DOMIT_CHECKER
endif

ifneq ($(max_players),0)
	CXXFLAGS += -DMAX_PLAYERS=$(max_players)
endif
ifneq ($(max_points),0)
	CXXFLAGS += -DMAX_POINTS=$(max_points)
endif
ifneq ($(max_rating),0)
	CXXFLAGS += -DMAX_RATING=$(max_rating)
endif
ifneq ($(max_rounds),0)
	CXXFLAGS += -DMAX_ROUNDS=$(max_rounds)
endif

ifeq ($(COMP),gcc)
	CXX=g++
	CXXFLAGS += -Wdouble-promotion -Wsuggest-final-types \
		-Wsuggest-final-methods -Wsuggest-override -Warray-bounds=2 \
		-Wduplicated-cond -Wtrampolines -Wconditionally-supported \
		-Wlogical-op -Wno-aggressive-loop-optimizations \
		-Wvector-operation-performance -Wno-maybe-uninitialized -Wuninitialized
endif
ifeq ($(COMP),clang)
	CXX=clang++
	CXXFLAGS += -Wno-uninitialized
endif

LDFLAGS += $(CXXFLAGS)

.PHONY: build clean
build: bbpPairings.exe

makefile_directory:=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))

clean:
	$(RM) bbpPairings.exe
	find $(makefile_directory) -name \*.o -type f -delete
	find $(makefile_directory) -name \*.d -type f -delete

default:
	build

bbpPairings.exe: $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

-include $(OBJECTS:%.o=%.d)
