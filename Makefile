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

COMPILER_FLAGS += -std=c++17 -I. -m$(bits) -MD -MP -Wpedantic -pedantic-errors \
	-Wall -Wextra -Wstrict-overflow=4 -Wundef -Wshadow -Wcast-qual \
	-Wcast-align -Wmissing-declarations -Wredundant-decls -Wvla \
	-Wno-unused-parameter -Wno-sign-compare -Wno-overflow -Wno-shadow

VERSION_INFO="$(shell git describe --exact-match 2> /dev/null)"
ifeq ($(VERSION_INFO),"")
	VERSION_INFO="non-release build $(shell git describe 2> /dev/null)"
endif
COMPILER_FLAGS += -DVERSION_INFO=$(VERSION_INFO)

ifeq ($(static),yes)
	LDFLAGS += -static
endif

ifeq ($(profile),yes)
	COMPILER_FLAGS += -ggdb
	CXXFLAGS += -O3
	ifneq ($(debug),yes)
		COMPILER_FLAGS += -DNDEBUG
	endif
else
	ifeq ($(debug),yes)
		COMPILER_FLAGS += -ggdb
		ifeq ($(optimize),yes)
			CXXFLAGS += -Og
		endif
	else
		COMPILER_FLAGS += -DNDEBUG
		LDFLAGS += -s
		ifeq ($(optimize),yes)
			CXXFLAGS += -O3
		endif
	endif
endif

ifneq ($(burstein),yes)
	COMPILER_FLAGS += -DOMIT_BURSTEIN
endif
ifneq ($(dutch),yes)
	COMPILER_FLAGS += -DOMIT_DUTCH
endif
ifneq ($(engine_comparison),yes)
	COMPILER_FLAGS += -DOMIT_GENERATOR -DOMIT_CHECKER
endif

ifneq ($(max_players),0)
	COMPILER_FLAGS += -DMAX_PLAYERS=$(max_players)
endif
ifneq ($(max_points),0)
	COMPILER_FLAGS += -DMAX_POINTS=$(max_points)
endif
ifneq ($(max_rating),0)
	CCOMPILER_FLAGS += -DMAX_RATING=$(max_rating)
endif
ifneq ($(max_rounds),0)
	COMPILER_FLAGS += -DMAX_ROUNDS=$(max_rounds)
endif

ifeq ($(COMP),)
	COMP=gcc
endif

ifeq ($(COMP),gcc)
	COMPILER_FLAGS += -Wdouble-promotion -Wsuggest-final-types \
		-Wsuggest-final-methods -Wsuggest-override -Warray-bounds=2 \
		-Wduplicated-cond -Wtrampolines -Wconditionally-supported \
		-Wlogical-op -Wno-aggressive-loop-optimizations \
		-Wvector-operation-performance -Wno-maybe-uninitialized -Wuninitialized

	ifeq ($(HOST),Windows)
		ifeq ($(bits),32)
			CXX=i686-w64-mingw32-g++
		else
			CXX=x86_64-w64-mingw32-g++
		endif
		ifeq ($(optimize),yes)
			COMPILER_FLAGS += -flto
		endif
		CXXFLAGS += $(COMPILER_FLAGS)
	else
		CXX=g++
		CXXFLAGS += $(COMPILER_FLAGS)
		CXXFLAGS += -DFILESYSTEM
		ifeq ($(optimize),yes)
			CXXFLAGS += -flto
		endif
		LDFLAGS += -lstdc++fs
	endif
endif
ifeq ($(COMP),clang)
	CXX=clang++
	CXXFLAGS += $(COMPILER_FLAGS)
	CXXFLAGS += -DEXPERIMENTAL_FILESYSTEM -Wno-uninitialized
	ifeq ($(optimize),yes)
		CXXFLAGS += -flto
	endif
	LDFLAGS += -lstdc++fs
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
