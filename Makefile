DIR_LIB:=lib
include $(DIR_LIB)/make-pal/pal.mak
DIR_SRC:=src
DIR_TEST:=test
DIR_INCLUDE:=include
DIR_BUILD:=build
CC:=gcc
AR:=ar

MAIN_NAME:=swicc
MAIN_SRC:=$(wildcard $(DIR_SRC)/*.c) $(wildcard $(DIR_SRC)/dbg/*.c) $(wildcard $(DIR_SRC)/fs/*.c)
MAIN_OBJ:=$(MAIN_SRC:$(DIR_SRC)/%.c=$(DIR_BUILD)/$(MAIN_NAME)/%.o)
MAIN_DEP:=$(MAIN_OBJ:%.o=%.d)
MAIN_CC_FLAGS:=\
	-W \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-unused-parameter \
	-Wconversion \
	-Wshadow \
	-static \
	-fPIC \
	-O2 \
	-I$(DIR_INCLUDE) \
	-I$(DIR_LIB)/cjson \
	-L$(DIR_LIB)/cjson/build \
	-Wl,-whole-archive -lcjson -Wl,-no-whole-archive

TEST_SRC:=$(wildcard $(DIR_TEST)/$(DIR_SRC)/*.c) $(wildcard $(DIR_TEST)/$(DIR_SRC)/$(MAIN_NAME)/*.c) $(wildcard $(DIR_TEST)/$(DIR_SRC)/$(MAIN_NAME)/fs/*.c)
TEST_OBJ:=$(TEST_SRC:$(DIR_TEST)/$(DIR_SRC)/%.c=$(DIR_BUILD)/$(DIR_TEST)/%.o)
TEST_DEP:=$(TEST_OBJ:%.o=%.d)
TEST_CC_FLAGS:=\
	-W \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-unused-parameter \
	-Wconversion \
	-Wshadow \
	-O2 \
	-I$(DIR_INCLUDE) \
	-Itest/$(DIR_INCLUDE) \
	-I$(DIR_LIB)/cjson \
	-I$(DIR_LIB)/tau \
	-I.. \
	-L$(DIR_BUILD) \
	-lswicc

all: main test
.PHONY: all

main: $(DIR_BUILD) $(DIR_BUILD)/$(MAIN_NAME)/dbg $(DIR_BUILD)/$(MAIN_NAME)/fs $(DIR_BUILD)/cjson $(DIR_BUILD)/$(LIB_PREFIX)$(MAIN_NAME).$(EXT_LIB_STATIC)
main-dbg-asan: MAIN_CC_FLAGS+=-fsanitize=address
main-dbg-asan: main-dbg
main-dbg: MAIN_CC_FLAGS+=-g -DDEBUG
main-dbg: main
.PHONY: main main-dbg main-dbg-asan

test: main-dbg $(DIR_BUILD) $(DIR_BUILD)/tmp $(DIR_BUILD)/$(DIR_TEST) $(DIR_BUILD)/$(DIR_TEST)/$(MAIN_NAME) $(DIR_BUILD)/$(DIR_TEST)/$(MAIN_NAME)/fs $(DIR_BUILD)/$(DIR_TEST).$(EXT_BIN)
test-dbg: TEST_CC_FLAGS+=-g -DDEBUG -fsanitize=address
test-dbg: test
.PHONY: test test-dbg

# Create the swICC static lib.
$(DIR_BUILD)/$(LIB_PREFIX)$(MAIN_NAME).$(EXT_LIB_STATIC): $(DIR_LIB)/cjson/build/libcjson.a $(MAIN_OBJ)
	cd $(DIR_BUILD)/cjson && $(AR) -x ../../$(DIR_LIB)/cjson/build/libcjson.a
	$(AR) -rcs $(@) $(MAIN_OBJ) $(DIR_BUILD)/cjson/*

# Create the test binary.
$(DIR_BUILD)/$(DIR_TEST).$(EXT_BIN): $(DIR_BUILD)/$(LIB_PREFIX)$(MAIN_NAME).$(EXT_LIB_STATIC) $(TEST_OBJ)
	$(CC) $(TEST_OBJ) -o $(@) $(TEST_CC_FLAGS)

# Build cjson lib.
$(DIR_LIB)/cjson/build/libcjson.a:
	$(call pal_mkdir,$(DIR_LIB)/cjson/build)
	cd $(DIR_LIB)/cjson/build && cmake -G "$(CMAKE_GENERATOR)" -DCMAKE_C_COMPILER=$(CC) -DCMAKE_CXX_COMPILER=$(CXX) -DCMAKE_MAKE_PROGRAM=$(MAKE) \
		-DENABLE_CJSON_TEST=Off -DENABLE_CJSON_UTILS=Off -DENABLE_TARGET_EXPORT=On \
		-DENABLE_CUSTOM_COMPILER_FLAGS=Off -DENABLE_VALGRIND=Off -DENABLE_SANITIZERS=Off \
		-DENABLE_SAFE_STACK=Off -DBUILD_SHARED_LIBS=Off -DBUILD_SHARED_AND_STATIC_LIBS=On \
		-DENABLE_LOCALES=On -DENABLE_CJSON_VERSION_SO=Off ..
	cd $(DIR_LIB)/cjson/build && $(MAKE) -j

# Compile source files to object files.
$(DIR_BUILD)/$(MAIN_NAME)/%.o: $(DIR_SRC)/%.c
	$(CC) $(<) -o $(@) $(MAIN_CC_FLAGS) -c -MMD
$(DIR_BUILD)/$(DIR_TEST)/%.o: $(DIR_TEST)/$(DIR_SRC)/%.c
	$(CC) $(<) -o $(@) $(TEST_CC_FLAGS) -c -MMD

# Recompile source files after a header they include changes.
-include $(MAIN_DEP)
-include $(TEST_DEP)

$(DIR_BUILD) $(DIR_BUILD)/$(MAIN_NAME)/dbg $(DIR_BUILD)/$(MAIN_NAME)/fs $(DIR_BUILD)/cjson $(DIR_BUILD)/tmp $(DIR_BUILD)/$(DIR_TEST) $(DIR_BUILD)/$(DIR_TEST)/$(MAIN_NAME) $(DIR_BUILD)/$(DIR_TEST)/$(MAIN_NAME)/fs:
	$(call pal_mkdir,$(@))
clean:
	$(call pal_rmdir,$(DIR_BUILD))
	$(call pal_rmdir,$(DIR_LIB)/cjson/build)
.PHONY: clean
