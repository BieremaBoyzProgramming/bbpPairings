
SRC = src
OBJ = build

SOURCES = $(shell find $(SRC) -name "*.cpp")
OBJECTS = $(patsubst $(SRC)/%.cpp, $(OBJ)/%.o, $(SOURCES))

# Flags specifying which Swiss systems to include.
burstein = yes
dutch = yes

# Flag indicating whether the tournament checker and random tournament generator
# should be included
engine_comparison = yes

# The maximum type sizes that the build should attempt to support.
# Default values are set/computed in tournament/tournament.h based on the
# constraints imposed by the TRF(x) format and build limitations.
# max_players = 9999
# Points are specified in tenths. So a value of 999 means that the max total
# score of a player (with acceleration) is 99.9.
# max_points = 999
# max_rating = 9999
# max_rounds = 99

# bits = 64
debug = no
profile = no
optimize = yes
static = no
COMP = gcc

optional_cxxflags = -std=c++20

optional_cxxflags +=  -Wpedantic -pedantic-errors -Wall -Wextra \
	-Wstrict-overflow=4 -Wundef -Wshadow -Wcast-qual -Wcast-align \
	-Wmissing-declarations -Wredundant-decls -Wvla -Wno-unused-parameter \
	-Wno-sign-compare -Wno-overflow -Wno-shadow

ifdef bits
	optional_cxxflags += -m$(bits)
endif

version = $(shell git describe --exact-match 2> /dev/null)
version_info = "$(version)"
ifeq ($(version),)
	version = $(shell git describe 2> /dev/null)
	version_info = "non-release build$(if $(version), )$(version)"
endif
optional_cxxflags += -DVERSION_INFO=$(version_info)

ifeq ($(static),yes)
	optional_ldflags += -static
endif

ifeq ($(profile),yes)
	optional_cxxflags += -ggdb
	optional_cxxflags += -O3
	ifneq ($(debug),yes)
		optional_cxxflags += -DNDEBUG
	endif
else
	ifeq ($(debug),yes)
		optional_cxxflags += -ggdb
		ifeq ($(optimize),yes)
			optional_cxxflags += -Og
		endif
	else
		optional_cxxflags += -DNDEBUG
		optional_ldflags += -s
		ifeq ($(optimize),yes)
			optional_cxxflags += -O3
		endif
	endif
endif

ifeq ($(optimize),yes)
	optional_cxxflags += -flto
endif

ifneq ($(burstein),yes)
	optional_cxxflags += -DOMIT_BURSTEIN
endif
ifneq ($(dutch),yes)
	optional_cxxflags += -DOMIT_DUTCH
endif
ifneq ($(engine_comparison),yes)
	optional_cxxflags += -DOMIT_GENERATOR -DOMIT_CHECKER
endif

ifdef max_players
	optional_cxxflags += -DMAX_PLAYERS=$(max_players)
endif
ifdef max_points
	optional_cxxflags += -DMAX_POINTS=$(max_points)
endif
ifdef max_rating
	optional_cxxflags += -DMAX_RATING=$(max_rating)
endif
ifdef max_rounds
	optional_cxxflags += -DMAX_ROUNDS=$(max_rounds)
endif

ifeq ($(COMP),gcc)
	CXX=g++
	optional_cxxflags += -Wpedantic -pedantic-errors -Wall -Wextra \
		-Wstrict-overflow=4 -Wundef -Wshadow -Wcast-qual -Wcast-align \
		-Wmissing-declarations -Wredundant-decls -Wvla -Wno-unused-parameter \
		-Wno-sign-compare -Wno-overflow -Wno-shadow -Wdouble-promotion \
		-Wsuggest-final-types -Wsuggest-final-methods -Wsuggest-override \
		-Warray-bounds=2 -Wduplicated-cond -Wtrampolines -Wconditionally-supported \
		-Wlogical-op -Wno-aggressive-loop-optimizations \
		-Wvector-operation-performance -Wno-maybe-uninitialized -Wuninitialized
endif
ifeq ($(COMP),clang)
	CXX=clang++
	optional_cxxflags += -Weverything
endif

CXXFLAGS = $(optional_cxxflags)

LDFLAGS = $(optional_ldflags) $(CXXFLAGS)

ifeq ($(COMP),gcc)
	target = $(shell g++ -v 2>&1 | sed -n -e 's/Target: [^-]*-\(.*\)/\1/p')
	prefix = $(shell g++ -v 2>&1 | sed -n -e 's/.*--prefix=\([^ ]*\).*/\1/p')
	thread_model = $(shell g++ -v 2>&1 | sed -n -e 's/Thread model: \(.*\)/\1/p')
	ifeq ($(target),w64-mingw32)
		ifeq ($(thread_model),win32)
			license_target = license-w64-mingw32-without-static-winpthreads
		endif
		ifneq ($(static),yes)
			license_target = license-w64-mingw32-without-static-winpthreads
		endif
	endif
endif

ifeq ($(target),w64-mingw32)
	host = windows
endif

.DELETE_ON_ERROR:

all: bbpPairings.exe
.PHONY: all

dist_name = bbpPairings$(if $(version),-)$(version)

dist_extension = tar.gz
ifeq ($(host),windows)
	dist_extension = zip
endif

dist_zip = $(dist_name).$(dist_extension)

dist: $(dist_zip)
.PHONY: dist

clean:
	$(RM) $(dist_zip)
	$(RM) -r $(dist_name)
	$(RM) bbpPairings.exe
	$(RM) -r build
.PHONY: clean

$(OBJ)/%.o: $(SRC)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) -o $@ $^ -c -I$(SRC) -MMD -MP $(CXXFLAGS)

$(OBJ)/bbpPairings.exe: $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

bbpPairings.exe: $(OBJ)/bbpPairings.exe
	cp $(OBJ)/bbpPairings.exe $@

-include $(OBJECTS:%.o=%.d)

$(dist_name)/:
	mkdir -p $(dist_name)

license-w64-mingw32-without-static-winpthreads: \
		$(OBJ)/bbpPairings.exe \
		packaging/mingw-w64/COPYING.MinGW-w64-runtime.txt.patch \
		$(prefix)/share/licenses/crt/COPYING.MinGW-w64-runtime.txt \
		| $(dist_name)/
	$(if \
		$(or \
			$(filter win32,$(thread_model)), \
			$(findstring winpthread,$(shell ldd $(OBJ)/bbpPairings.exe))), \
		, \
		$(error \
			License file generation for statically-linked winpthreads has not been \
				implemented))
	mkdir -p $(dist_name)
	cp LICENSE.txt $(dist_name)/LICENSE.txt
	echo "" >> $(dist_name)/LICENSE.txt
	patch -u $(prefix)/share/licenses/crt/COPYING.MinGW-w64-runtime.txt \
		packaging/mingw-w64/COPYING.MinGW-w64-runtime.txt.patch -o /dev/stdout -s \
		| cat >> $(dist_name)/LICENSE.txt
.PHONY: license-w64-mingw32-without-static-winpthreads

license-not-supported:
	$(error Automatic license generation is not supported for this configuration.)
.PHONY: license-not-supported

$(dist_name)/LICENSE.txt: $(license_target)

$(dist_name)/bbpPairings.exe: $(OBJ)/bbpPairings.exe | $(dist_name)/
	cp $(OBJ)/bbpPairings.exe $@

$(dist_name).zip: $(dist_name)/bbpPairings.exe $(dist_name)/LICENSE.txt
	zip -r $(dist_zip) $(dist_name)
