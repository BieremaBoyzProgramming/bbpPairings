
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

optional_cxxflags = \
	-std=c++20 \
	-ftabstop=2 \
	-Werror \
	-Wfatal-errors \
	-pedantic \
	-pedantic-errors \
	-Wall \
	-Walloca \
	-Wcast-qual \
	-Wctor-dtor-privacy \
	-Wdisabled-optimization \
	-Wdouble-promotion \
	-Wextra \
	-Wextra-semi \
	-Wformat=2 \
	-Winvalid-pch \
	-Wmismatched-tags \
	-Wmissing-braces \
	-Wmissing-declarations \
	-Wmissing-include-dirs \
	-Wnull-dereference \
	-Woverloaded-virtual \
	-Wpacked \
	-Wpointer-arith \
	-Wredundant-decls \
	-Wsign-promo \
	-Wstrict-overflow=4 \
	-Wsuggest-override \
	-Wswitch-default \
	-Wundef \
	-Wunknown-pragmas \
	-Wunused-macros \
	-Wvla \
	-Wzero-as-null-pointer-constant \
	-Wno-free-nonheap-object \
	-Wno-overflow \
	-Wno-sign-compare
# Omitted because they were being triggered:
# -Waggregate-return
# -Wconversion
# -Wdate-time
# -Weffc++
# -Wfloat-conversion
# -Wfloat-equal
# -Winline
# -Wlong-long
# -Wnon-virtual-dtor
# -Wold-style-cast
# -Wpadded
# -Wshadow
# -Wsign-conversion
# -Wstrict-overflow=5
# -Wswitch-enum
# -Wsystem-headers

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

ifeq ($(COMP),gcc)
	comp_version := \
		$(shell g++ -v 2>&1 | sed -n 's/^gcc version \([0-9]*\.[0-9]*\.[0-9]*\).*/\1/p')
	target = $(shell g++ -v 2>&1 | sed -n -e 's/Target: [^-]*-\(.*\)/\1/p')
	prefix = $(shell g++ -v 2>&1 | sed -n -e 's/.*--prefix=\([^ ]*\).*/\1/p')
	thread_model = $(shell g++ -v 2>&1 | sed -n -e 's/Thread model: \(.*\)/\1/p')
	ifeq ($(target),w64-mingw32)
		license_id = w64-mingw32
		target_os = windows
	else ifeq ($(target),pc-linux-gnu)
		license_id = linux
		target_os = linux
	endif
endif
ifeq ($(COMP),clang)
	target = $(shell clang++ -v 2>&1 | sed -n -e 's/Target: [^-]*-\(.*\)/\1/p')
	ifeq ($(target),w64-windows-gnu)
		target_os = windows
	else ifeq ($(target),pc-linux-gnu)
		target_os = linux
	endif
endif

ifeq ($(static),yes)
	ifeq ($(target_os),linux)
		optional_ldflags += -static-libstdc++ -static-libgcc
	else
		optional_ldflags += -static
	endif
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
	# I haven't gotten LTO to work with Clang on mingw-w64.
	ifneq ($(COMP),clang)
		optional_cxxflags += -flto
	else ifneq ($(target),w64-windows-gnu)
		optional_cxxflags += -flto
	endif
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
	comp_version_major := \
		$(shell echo $(comp_version) | sed 's/\([0-9]*\)\..*/\1/')
	comp_version_at_least_11 := \
		$(shell [ $(comp_version_major) -ge 11 ] && echo 1)
	# The warning flags included for GCC are based on GCC 11.2.
	optional_cxxflags += \
		-Wabi=0 \
		-Waligned-new=all \
		-Walloc-zero \
		-Warray-bounds=2 \
		$(if $(comp_version_at_least_11),-Warray-parameter=2) \
		-Wattribute-alias=2 \
		-Wcast-align=strict \
		-Wcatch-value=3 \
		-Wconditionally-supported \
		$(if $(comp_version_at_least_11),-Wctad-maybe-unsupported) \
		-Wduplicated-cond \
		$(if $(comp_version_at_least_11),-Wenum-conversion) \
		-Wformat-overflow=2 \
		-Wformat-signedness \
		-Wformat-truncation=2 \
		-Wimplicit-fallthrough=3 \
		$(if $(comp_version_at_least_11),-Winvalid-imported-macros) \
		-Wlogical-op \
		-Wmultiple-inheritance \
		-Wnormalized=nfkc \
		-Wplacement-new=2 \
		-Wredundant-tags \
		-Wshadow=local \
		-Wstrict-null-sentinel \
		-Wstringop-overflow=4 \
		-Wsuggest-attribute=cold \
		-Wsuggest-attribute=format \
		-Wsuggest-attribute=malloc \
		-Wsuggest-attribute=noreturn \
		-Wsuggest-final-methods \
		-Wsuggest-final-types \
		-Wsync-nand \
		-Wtrampolines \
		-Wunsafe-loop-optimizations \
		-Wvector-operation-performance \
		-Wvirtual-inheritance \
		-Wno-maybe-uninitialized
	# Omitted because they were being triggered:
	# -Wabi-tag
	# -Warith-conversion
	# -Wduplicated-branches
	# -Wnamespaces
	# -Wsuggest-attribute=const
	# -Wsuggest-attribute=pure
	# -Wtemplates
	# -Wuseless-cast
endif
ifeq ($(COMP),clang)
	CXX=clang++
	# The warning flags included for Clang are based on Clang 13.
	optional_cxxflags += \
		-Wabstract-vbase-init \
		-Wanon-enum-enum-conversion \
		-Warc-repeated-use-of-weak \
		-Warray-bounds-pointer-arithmetic \
		-Wassign-enum \
		-Watomic-implicit-seq-cst \
		-Watomic-properties \
		-Wauto-import \
		-Wbad-function-cast \
		-Wbinary-literal \
		-Wbind-to-temporary-copy \
		-Wc++-compat \
		-Wc++11-extensions \
		-Wc++14-compat-pedantic \
		-Wc++14-extensions \
		-Wc++17-compat-pedantic \
		-Wc++17-extensions \
		-Wc++20-compat-pedantic \
		-Wc++20-extensions \
		-Wc99-compat \
		-Wc99-extensions \
		-Wcalled-once-parameter \
		-Wcast-align \
		-Wcast-function-type \
		-Wclass-varargs \
		-Wcomma \
		-Wcompound-token-split \
		-Wconsumed \
		-Wcovered-switch-default \
		-Wcstring-format-directive \
		-Wctad-maybe-unsupported \
		-Wcuda-compat \
		-Wdeprecated \
		-Wdeprecated-implementations \
		-Wdirect-ivar-access \
		-Wdisabled-macro-expansion \
		-Wdocumentation \
		-Wdocumentation-pedantic \
		-Wdtor-name \
		-Wduplicate-decl-specifier \
		-Wduplicate-enum \
		-Wduplicate-method-arg \
		-Wduplicate-method-match \
		-Wdynamic-exception-spec \
		-Wenum-conversion \
		-Wexit-time-destructors \
		-Wexpansion-to-defined \
		-Wexplicit-ownership-type \
		-Wextra-semi-stmt \
		-Wformat-non-iso \
		-Wformat-pedantic \
		-Wformat-type-confusion \
		-Wfour-char-constants \
		-Wgcc-compat \
		-Wglobal-constructors \
		-Wgnu \
		-Wheader-hygiene \
		-Widiomatic-parentheses \
		-Wimplicit-fallthrough \
		-Wimplicit-retain-self \
		-Wincomplete-module \
		-Winconsistent-missing-destructor-override \
		-Winvalid-or-nonexistent-directory \
		-Wlocal-type-template-args \
		-Wloop-analysis \
		-Wmain \
		-Wmax-tokens \
		-Wmethod-signatures \
		-Wmicrosoft \
		-Wmissing-noreturn \
		-Wmissing-prototypes \
		-Wmissing-variable-declarations \
		-Wnewline-eof \
		-Wnon-gcc \
		-Wnonportable-system-include-path \
		-Wnullable-to-nonnull-conversion \
		-Wobjc-interface-ivars \
		-Wobjc-messaging-id \
		-Wobjc-missing-property-synthesis \
		-Wobjc-property-assign-on-object-type \
		-Wobjc-signed-char-bool \
		-Wopenmp \
		-Wover-aligned \
		-Woverriding-method-mismatch \
		-Wpedantic-core-features \
		-Wpoison-system-directories \
		-Wpragmas \
		-Wpre-c2x-compat \
		-Wpre-openmp-51-compat \
		-Wprofile-instr-missing \
		-Wquoted-include-in-framework-header \
		-Wreceiver-forward-class \
		-Wredundant-parens \
		-Wreserved-identifier \
		-Wreserved-user-defined-literal \
		-Wselector \
		-Wshadow-all \
		-Wshift-sign-overflow \
		-Wsigned-enum-bitfield \
		-Wspir-compat \
		-Wstatic-in-inline \
		-Wstrict-potentially-direct-selector \
		-Wstrict-prototypes \
		-Wstrict-selector-match \
		-Wsuggest-destructor-override \
		-Wsuper-class-method-mismatch \
		-Wtautological-constant-in-range-compare \
		-Wthread-safety \
		-Wthread-safety-beta \
		-Wthread-safety-negative \
		-Wthread-safety-verbose \
		-Wundeclared-selector \
		-Wundef-prefix \
		-Wundefined-reinterpret-cast \
		-Wunguarded-availability \
		-Wunnamed-type-template-args \
		-Wunneeded-internal-declaration \
		-Wunreachable-code-aggressive \
		-Wunsupported-dll-base-class-template \
		-Wunused-member-function \
		-Wused-but-marked-unused \
		-Wvariadic-macros \
		-Wvector-conversion \
		-Wno-c++98-compat-local-type-template-args \
		-Wno-float-conversion \
		-Wno-implicit-int-float-conversion \
		-Wno-implicit-int-conversion \
		-Wno-shorten-64-to-32 \
		-Wno-sign-compare \
		-Wno-sign-conversion \
		-Wno-tautological-type-limit-compare \
		-Wno-uninitialized \
		-Wfloat-overflow-conversion \
		-Wfloat-zero-conversion \
		-Wimplicit-const-int-float-conversion \
		-Wsometimes-uninitialized \
		-Wstatic-self-init
	# Omitted because they were being triggered:
	# -Wc++11-compat-pedantic
	# -Wc++98-compat-pedantic
	# -Wconditional-uninitialized
	# -Wundefined-func-template
	# -Wuninitialized-const-reference
	# -Wunused-exception-parameter
	# -Wunused-template
	# -Wweak-template-vtables
	# -Wweak-vtables
endif

CXXFLAGS = $(optional_cxxflags)

LDFLAGS = $(optional_ldflags) $(CXXFLAGS)

.DELETE_ON_ERROR:

all: bbpPairings.exe
.PHONY: all

dist_name = bbpPairings$(if $(version),-)$(version)

dist_extension = tar.gz
ifeq ($(target_os),windows)
	dist_extension = zip
endif

dist_zip = $(dist_name).$(dist_extension)

dist: $(dist_zip)
.PHONY: dist

clean: clean-tests
	$(RM) -r bbpPairings*
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

$(dist_name)/bbpPairings.exe: $(OBJ)/bbpPairings.exe | $(dist_name)/
	cp $(OBJ)/bbpPairings.exe $@

ifeq ($(license_id),w64-mingw32)
$(dist_name)/LICENSE.txt: \
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
else ifeq ($(license_id),linux)
$(dist_name)/LICENSE.txt: $(OBJ)/bbpPairings.exe | $(dist_name)/
	cp LICENSE.txt $(dist_name)/LICENSE.txt
else
$(dist_name)/LICENSE.txt:
	$(error Automatic license generation is not supported for this configuration.)
endif

$(dist_name)/README.txt: README.txt
	cp README.txt $@

$(dist_name)/Apache-2.0.txt: Apache-2.0.txt
	cp Apache-2.0.txt $@

dist-targets: \
		$(dist_name)/bbpPairings.exe $(dist_name)/LICENSE.txt \
		$(dist_name)/README.txt $(dist_name)/Apache-2.0.txt
.PHONY: dist-targets

$(dist_name).zip: dist-targets
	zip -r $(dist_zip) $(dist_name)

$(dist_name).tar.gz: dist-targets
	tar -czf $(dist_zip) $(dist_name)

TEST = test

tests:
	$(MAKE) -C $(TEST)
.PHONY: tests

test: bbpPairings.exe
	$(MAKE) -C $(TEST) run
.PHONY: test

clean-tests:
	$(MAKE) -C $(TEST) clean
.PHONY: clean-tests
