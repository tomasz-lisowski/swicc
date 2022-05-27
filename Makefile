DIR_LIB:=lib
include $(DIR_LIB)/make-pal/pal.mak
DIR_SRC:=src
DIR_TEST:=test
DIR_INCLUDE:=include
DIR_BUILD:=build
CC:=gcc
AR:=ar

MAIN_NAME:=$(LIB_PREFIX)uicc
MAIN_SRC:=$(wildcard $(DIR_SRC)/*.c) $(wildcard $(DIR_SRC)/dbg/*.c) $(wildcard $(DIR_SRC)/fs/*.c)
MAIN_OBJ:=$(MAIN_SRC:$(DIR_SRC)/%.c=$(DIR_BUILD)/$(MAIN_NAME)/%.o)
MAIN_DEP:=$(MAIN_OBJ:%.o=%.d)
MAIN_CC_FLAGS:=-static -Werror -Wno-unused-parameter -W -Wall -Wextra -Wconversion -Wshadow -fPIC -O2 \
	-I$(DIR_INCLUDE) \
	-I$(DIR_LIB)/cjson \
	-L$(DIR_LIB)/cjson/build \
	-Wl,-whole-archive -lcjson -Wl,-no-whole-archive

TEST_NAME:=test
TEST_SRC:=$(wildcard $(DIR_TEST)/*.c) $(wildcard $(DIR_TEST)/fs/*.c)
TEST_OBJ:=$(TEST_SRC:$(DIR_TEST)/%.c=$(DIR_BUILD)/$(TEST_NAME)/%.o)
TEST_DEP:=$(TEST_OBJ:%.o=%.d)
TEST_CC_FLAGS:=-Werror -Wno-unused-parameter -W -Wall -Wextra -Wconversion -Wshadow -O2 \
	-g -DDEBUG -fsanitize=address \
	-I$(DIR_INCLUDE) \
	-I$(DIR_LIB)/cjson \
	-I$(DIR_LIB)/tau \
	-I.. \
	-L$(DIR_BUILD) \
	-luicc

all: main test

main: main-lib main-fast
main-fast: $(DIR_BUILD) $(DIR_BUILD)/$(MAIN_NAME)/dbg $(DIR_BUILD)/$(MAIN_NAME)/fs $(DIR_BUILD)/cjson $(DIR_BUILD)/$(MAIN_NAME).$(EXT_LIB_STATIC)
main-dbg: MAIN_CC_FLAGS+=-g -DDEBUG -fsanitize=address
main-dbg: main
main-lib: cjson

test: main test-lib test-fast
test-fast: main-fast $(DIR_BUILD) $(DIR_BUILD)/$(TEST_NAME) $(DIR_BUILD)/$(TEST_NAME)/fs $(DIR_BUILD)/$(TEST_NAME).$(EXT_BIN)
test-lib: tau

# Create the UICC static lib.
$(DIR_BUILD)/$(MAIN_NAME).$(EXT_LIB_STATIC): $(MAIN_OBJ)
	cd $(DIR_BUILD)/cjson && $(AR) -x ../../$(DIR_LIB)/cjson/build/libcjson.a
	$(AR) -rcs $(@) $(^) $(DIR_BUILD)/cjson/*

# Create the test binary.
$(DIR_BUILD)/$(TEST_NAME).$(EXT_BIN): $(TEST_OBJ)
	$(CC) $(^) -o $(@) $(TEST_CC_FLAGS)

# Build cjson lib.
cjson:
	$(call pal_mkdir,$(DIR_LIB)/cjson/build)
	cd $(DIR_LIB)/cjson/build && cmake -G "$(CMAKE_GENERATOR)" -DCMAKE_C_COMPILER=$(CC) -DCMAKE_CXX_COMPILER=$(CXX) -DCMAKE_MAKE_PROGRAM=$(MAKE) \
		-DENABLE_CJSON_TEST=Off -DENABLE_CJSON_UTILS=Off -DENABLE_TARGET_EXPORT=On \
		-DENABLE_CUSTOM_COMPILER_FLAGS=Off -DENABLE_VALGRIND=Off -DENABLE_SANITIZERS=Off \
		-DENABLE_SAFE_STACK=Off -DBUILD_SHARED_LIBS=Off -DBUILD_SHARED_AND_STATIC_LIBS=On \
		-DENABLE_LOCALES=On -DENABLE_CJSON_VERSION_SO=Off ..
	cd $(DIR_LIB)/cjson/build && $(MAKE)

# Build tau lib.
tau:
	cd $(DIR_LIB)/tau && $(MAKE) -j

# Compile source files to object files.
$(DIR_BUILD)/$(MAIN_NAME)/%.o: $(DIR_SRC)/%.c
	$(CC) $(<) -o $(@) $(MAIN_CC_FLAGS) -c -MMD
$(DIR_BUILD)/$(TEST_NAME)/%.o: $(DIR_TEST)/%.c
	$(CC) $(<) -o $(@) $(TEST_CC_FLAGS) -c -MMD

# Recompile source files after a header they include changes.
-include $(MAIN_DEP)
-include $(TEST_DEP)

$(DIR_BUILD) $(DIR_BUILD)/$(MAIN_NAME)/dbg $(DIR_BUILD)/$(MAIN_NAME)/fs $(DIR_BUILD)/cjson $(DIR_BUILD)/$(TEST_NAME) $(DIR_BUILD)/$(TEST_NAME)/fs:
	$(call pal_mkdir,$(@))
clean:
	$(call pal_rmdir,$(DIR_LIB)/cjson/build)
	$(call pal_rmdir,$(DIR_LIB)/tau/build)
	$(call pal_rmdir,$(DIR_BUILD))

.PHONY: all main main-fast main-dbg main-lib test test-fast test-lib cjson tau clean
