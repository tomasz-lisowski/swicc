DIR_LIB:=lib
include $(DIR_LIB)/make-pal/pal.mak
DIR_SRC:=src
DIR_INCLUDE:=include
DIR_BUILD:=build
DIR_BUILD_LIB:=build-lib
CC:=gcc
AR:=ar

MAIN_NAME:=$(LIB_PREFIX)uicc
MAIN_SRC:=$(wildcard $(DIR_SRC)/*.c) $(wildcard $(DIR_SRC)/dbg/*.c)
MAIN_OBJ:=$(MAIN_SRC:$(DIR_SRC)/%.c=$(DIR_BUILD)/%.o)
MAIN_DEP:=$(MAIN_OBJ:%.o=%.d)
MAIN_CC_FLAGS:=-Werror -Wno-unused-parameter -W -Wall -Wextra -Wconversion -Wshadow \
	-fPIC -O2 \
	-I$(DIR_INCLUDE) -I$(DIR_BUILD_LIB)/cjson/$(DIR_INCLUDE) \
	-L$(DIR_BUILD_LIB)/cjson -lcjson
MAIN_AR_FLAGS:=-rcs

all: all-lib main
all-fast: main
all-dbg: MAIN_CC_FLAGS+=-g -DDEBUG
all-dbg: main
all-lib: cjson

# Create static library.
main: $(DIR_BUILD) $(DIR_BUILD)/dbg $(DIR_BUILD)/$(MAIN_NAME).$(EXT_LIB_STATIC)
$(DIR_BUILD)/$(MAIN_NAME).$(EXT_LIB_STATIC): $(MAIN_OBJ)
	$(AR) $(MAIN_AR_FLAGS) $(@) $(^)

# Build cjson.
cjson: $(DIR_BUILD_LIB)
	$(call pal_mkdir,$(DIR_LIB)/cjson/build)
	cd $(DIR_LIB)/cjson/build && cmake -G "$(CMAKE_GENERATOR)" -DCMAKE_C_COMPILER=$(CC) -DCMAKE_CXX_COMPILER=$(CXX) -DCMAKE_MAKE_PROGRAM=$(MAKE) \
		-DENABLE_CJSON_TEST=Off -DENABLE_CJSON_UTILS=Off -DENABLE_TARGET_EXPORT=On \
		-DENABLE_CUSTOM_COMPILER_FLAGS=Off -DENABLE_VALGRIND=Off -DENABLE_SANITIZERS=Off \
		-DENABLE_SAFE_STACK=Off -DBUILD_SHARED_LIBS=Off -DBUILD_SHARED_AND_STATIC_LIBS=On \
		-DENABLE_LOCALES=On -DENABLE_CJSON_VERSION_SO=Off ..
	cd $(DIR_LIB)/cjson/build && $(MAKE)
	$(call pal_mkdir,$(DIR_BUILD_LIB)/cjson)
	$(call pal_mkdir,$(DIR_BUILD_LIB)/cjson/$(DIR_INCLUDE))
	$(call pal_cp,$(DIR_LIB)/cjson/build/libcjson.a,$(DIR_BUILD_LIB)/cjson/$(LIB_PREFIX)cjson.$(EXT_LIB_STATIC))
	$(call pal_cp,$(DIR_LIB)/cjson/cJSON.h,$(DIR_BUILD_LIB)/cjson/$(DIR_INCLUDE))

# Compile source files to object files.
$(DIR_BUILD)/%.o: $(DIR_SRC)/%.c
	$(CC) $(<) -o $(@) $(MAIN_CC_FLAGS) -c -MMD

# Recompile source files after a header they include changes.
-include $(MAIN_DEP)

$(DIR_BUILD) $(DIR_BUILD)/dbg $(DIR_BUILD_LIB):
	$(call pal_mkdir,$(@))
clean:
	$(call pal_rmdir,$(DIR_BUILD))
	$(call pal_rmdir,$(DIR_BUILD_LIB))
	$(call pal_rmdir,$(DIR_LIB)/cjson/build)

.PHONY: all all-fast all-dbg all-lib main cjson clean
