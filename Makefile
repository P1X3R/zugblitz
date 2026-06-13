CC       ?= clang
MODE     ?= release
DIST     ?= dist

COMMON_FLAGS = -Wall -Wextra -Wpedantic -std=c11
LDFLAGS      =

ifeq ($(findstring mingw,$(CC)),mingw)
    HOST_WIN := 1
    EXE      := .exe
else
    HOST_WIN :=
    EXE      :=
    COMMON_FLAGS += -D_POSIX_C_SOURCE=200808L
endif

ifeq ($(MODE),debug)
    SANITIZERS := address,undefined
    CFLAGS  := $(COMMON_FLAGS) -O1 -g -fsanitize=$(SANITIZERS) \
               -fno-omit-frame-pointer
    LDFLAGS += -fsanitize=$(SANITIZERS)

else ifeq ($(MODE),release)
    CFLAGS  := $(COMMON_FLAGS) -O3 -march=native -DNDEBUG -flto
    LDFLAGS += -flto

else ifeq ($(MODE),portable)
    CFLAGS  := $(COMMON_FLAGS) -O3 -march=x86-64-v2 -mtune=generic -DNDEBUG
    LDFLAGS += -s -static
    ifeq ($(HOST_WIN),1)
        LDFLAGS += -static-libgcc -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic
    endif
else
    $(error Unknown MODE '$(MODE)' (expected 'debug', 'release', or 'portable'))
endif

SRC := $(filter-out src/bake.c src/main.c,$(wildcard src/*.c))
OBJ := $(SRC:.c=.o)

all: zugblitz

zugblitz$(EXE): src/main.o $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

bake$(EXE): src/bake.o $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) src/main.o src/bake.o dist/*

export: zugblitz$(EXE)
	@mkdir -p $(DIST)
	# determine TARGET_OS and TARGET_ARCH from the compiler
	$(eval TARGET_OS := $(shell $(CC) -dumpmachine | cut -d- -f3))
	$(eval TARGET_ARCH := $(shell $(CC) -dumpmachine | cut -d- -f1))
	mv zugblitz$(EXE) $(DIST)/zugblitz-$(TARGET_OS)-$(TARGET_ARCH)$(EXE)
	strip $(DIST)/zugblitz-*

.PHONY: all clean export bake zugblitz
