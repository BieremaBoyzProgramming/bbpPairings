OBJECTS = main.o fileformats/generatorconfiguration.o fileformats/trf.o \
	matching/computer.o matching/detail/blossom.o matching/detail/graph.o \
	matching/detail/parentblossom.o matching/detail/rootblossom.o \
	swisssystems/burstein.o swisssystems/common.o tournament/checker.o \
	tournament/generator.o tournament/tournament.o

bits = 64
debug = no
max_players = 0
max_points = 0
max_rating = 0
max_rounds = 0
static = no
optimize = yes

DEPEND_FLAGS += -std=c++14 -I.
COMPILER_FLAGS += -m$(bits) -Wpedantic -pedantic-errors -Wall -Wextra \
	-Wuninitialized -Wstrict-overflow=4 -Wundef -Wshadow -Wcast-qual \
	-Wcast-align -Wmissing-declarations -Wredundant-decls -Wvla \
	-Wno-unused-parameter -Wno-sign-compare -Wno-maybe-uninitialized -Wno-overflow

VERSION_INFO="$(shell git describe --exact-match 2> /dev/null)"
ifeq ($(VERSION_INFO),"")
	VERSION_INFO="non-release build $(shell git describe)"
endif
COMPILER_FLAGS += -DVERSION_INFO=$(VERSION_INFO)

ifeq ($(static),yes)
	LDFLAGS += -static
endif

ifeq ($(debug),yes)
	COMPILER_FLAGS += -g
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

COMPILER_FLAGS += $(DEPEND_FLAGS)

ifeq ($(COMP),)
	COMP=gcc
endif

ifeq ($(COMP),gcc)
	COMPILER_FLAGS += -Wdouble-promotion -Wsuggest-final-types \
		-Wsuggest-final-methods -Wsuggest-override -Warray-bounds=2 \
		-Wduplicated-cond -Wtrampolines -Wconditionally-supported \
		-Wzero-as-null-pointer-constant -Wlogical-op \
		-Wno-aggressive-loop-optimizations -Wvector-operation-performance

	ifeq ($(HOST),Windows)
		CXX=x86_64-w64-mingw32-g++
		UNOPTIMIZED_FLAGS = $(COMPILER_FLAGS)
		CXXFLAGS += $(COMPILER_FLAGS)
	else
		CXX=g++
		COMPILER_FLAGS += -DEXPERIMENTAL_FILESYSTEM
		ifeq ($(optimize),yes)
			CXXFLAGS += -flto
		endif
		LDFLAGS += -lstdc++fs
		CXXFLAGS += $(COMPILER_FLAGS)
		UNOPTIMIZED_FLAGS = $(CXXFLAGS)
	endif
endif

LDFLAGS += $(CXXFLAGS)

.PHONY: build clean
build: bbpPairings.exe .depend

clean:
	$(RM) bbpPairings.exe .dep.inc .depend
	find . -name \*.o -type f -delete

default:
	build

bbpPairings.exe: $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

.depend:
	-@$(CXX) $(DEPEND_FLAGS) -MM $(OBJECTS:.o=.cpp) > $@

# A bug in mingw-w64 causes compilation of tournament/generator.o with
# optimization to fail.
tournament/generator.o: CXXFLAGS := $(UNOPTIMIZED_FLAGS)

-include .depend
