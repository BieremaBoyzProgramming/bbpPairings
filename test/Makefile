# Detect if g++ is actually clang (common on macOS)
is_apple_clang := $(shell g++ -v 2>&1 | grep -q "Apple clang version" && echo yes)

# Basic warning flags that work on both gcc and clang
optional_cxxflags = \
	-std=c++20 \
	-ftabstop=2 \
	-Werror \
	-Wfatal-errors \
	-pedantic \
	-pedantic-errors \
	-Wall \
	-Wextra \
	-Wcast-qual \
	-Wctor-dtor-privacy \
	-Wdisabled-optimization \
	-Wdouble-promotion \
	-Wextra-semi \
	-Wformat=2 \
	-Winvalid-pch \
	-Wmismatched-tags \
	-Wmissing-braces \
	-Wmissing-include-dirs \
	-Wnull-dereference \
	-Woverloaded-virtual \
	-Wpacked \
	-Wpointer-arith \
	-Wredundant-decls \
	-Wsign-promo \
	-Wsuggest-override \
	-Wswitch-default \
	-Wundef \
	-Wunused-macros \
	-Wvla \
	-Wzero-as-null-pointer-constant

# Add compiler-specific flags
ifeq ($(is_apple_clang),yes)
	# Minimal flags for macOS clang
	optional_cxxflags += \
		-Wno-deprecated-declarations \
		-Wno-uninitialized \
		-Wno-sign-compare
else
	# GCC-specific flags (for real GCC on Linux)
	optional_cxxflags += \
		-Wabi=0 \
		-Waligned-new=all \
		-Walloc-zero \
		-Walloca \
		-Warith-conversion \
		-Warray-bounds=2 \
		-Warray-parameter=2 \
		-Wattribute-alias=2 \
		-Wcast-align=strict \
		-Wcatch-value=3 \
		-Wconditionally-supported \
		-Wconversion \
		-Wctad-maybe-unsupported \
		-Wdate-time \
		-Wduplicated-branches \
		-Wduplicated-cond \
		-Weffc++ \
		-Wenum-conversion \
		-Wfloat-equal \
		-Wformat-overflow=2 \
		-Wformat-signedness \
		-Wformat-truncation=2 \
		-Wimplicit-fallthrough=3 \
		-Winline \
		-Winvalid-imported-macros \
		-Wlogical-op \
		-Wlong-long \
		-Wmultiple-inheritance \
		-Wnormalized=nfkc \
		-Wold-style-cast \
		-Wpadded \
		-Wplacement-new=2 \
		-Wredundant-tags \
		-Wshadow \
		-Wsign-conversion \
		-Wstrict-null-sentinel \
		-Wstrict-overflow=5 \
		-Wstringop-overflow=4 \
		-Wsuggest-attribute=cold \
		-Wsuggest-attribute=const \
		-Wsuggest-attribute=format \
		-Wsuggest-attribute=malloc \
		-Wsuggest-attribute=noreturn \
		-Wsuggest-attribute=pure \
		-Wsuggest-final-methods \
		-Wsuggest-final-types \
		-Wswitch-enum \
		-Wsync-nand \
		-Wtrampolines \
		-Wunsafe-loop-optimizations \
		-Wuseless-cast \
		-Wvector-operation-performance \
		-Wvirtual-inheritance
endif
# Omitted because they were being triggered:
# -Wsystem-headers
# -Wabi-tag
# -Waggregate-return
# -Wmissing-declarations
# -Wnamespaces
# -Wtemplates
# Not recognized by compiler:
# -Wbidi-chars=any
# -Winterference-size
# -Wopenacc-parallelism

ifeq ($(shell uname -o),Msys)
	optional_cxxflags += -static
endif

CXX=g++

CXXFLAGS = $(optional_cxxflags)

TESTS = tests

TEST_SOURCES = $(shell find $(TESTS) -name "*.cpp")
TEST_IDS = $(patsubst $(TESTS)/%.cpp, %, $(TEST_SOURCES))

.DELETE_ON_ERROR:

all: bbpPairingsTests.exe
.PHONY: all

test-includes.h: $(TESTS)
	echo > $@
	$(foreach \
		test_id, \
		$(TEST_IDS), \
		echo "#define TEST_ID $(test_id)" >> $@; \
			echo "#include <tests/$(test_id).cpp>" >> $@; \
			echo "#undef TEST_ID" >> $@;)
	echo >> $@
	echo "int runTests(const testing::Context &context)" >> $@
	echo { >> $@
	echo "  BEFORE_RUNNING_TESTS" >> $@
	$(foreach test_id, $(TEST_IDS), echo "  RUN_TEST($(test_id))" >> $@;)
	echo "  AFTER_RUNNING_TESTS" >> $@
	echo } >> $@

bbpPairingsTests.exe: test-includes.h main.cpp
	$(CXX) -o $@ -I. -MMD -MP main.cpp $(CXXFLAGS)

-include bbpPairingsTests.d

run: bbpPairingsTests.exe
	./bbpPairingsTests.exe ../bbpPairings.exe $(TESTS)
.PHONY: run

clean:
	$(RM) -r $(TESTS)/*.output
	$(RM) -r bbpPairingsTests.*
	$(RM) -r test-includes.h
.PHONY: clean
