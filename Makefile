# emu_manager — aarch64 native build for ROCKNIX / iux (GKD 640x480)
#
# On device (if gcc+dev libs exist):
#   make native && make install-native
#
# Cross-compile from Linux PC:
#   make aarch64 CROSS=aarch64-linux-gnu-

TARGET   := emumanager
SRC      := src/main.c src/scan.c src/fs_util.c src/ui.c
OBJ      := $(SRC:.c=.o)

CROSS    ?=
CC       := $(CROSS)gcc
PKG_CONF := $(CROSS)pkg-config

SDL_CFLAGS  ?= $(shell $(PKG_CONF) --cflags sdl2 SDL2_ttf 2>/dev/null)
SDL_LDFLAGS ?= $(shell $(PKG_CONF) --libs sdl2 SDL2_ttf 2>/dev/null)

# 交叉编译时若 pkg-config 失败，可在命令行覆盖，例如：
# make aarch64 SDL_CFLAGS="-I/usr/include/aarch64-linux-gnu/SDL2" \
#   SDL_LDFLAGS="-L/usr/lib/aarch64-linux-gnu -lSDL2 -lSDL2_ttf"
ifeq ($(strip $(SDL_CFLAGS)),)
  SDL_CFLAGS := -I/usr/include/SDL2
endif
ifeq ($(strip $(SDL_LDFLAGS)),)
  SDL_LDFLAGS := -lSDL2 -lSDL2_ttf
endif

CFLAGS  ?= -O2 -Wall -Wextra -std=c11
CFLAGS  += -Iinclude $(SDL_CFLAGS) -D_DEFAULT_SOURCE
LDFLAGS += $(SDL_LDFLAGS) -lm

INSTALL_DIR := /storage/iux/emumanager
SECTION_SRC := packaging/emumanager

.PHONY: all clean native aarch64 install-native install-section

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

native: CROSS :=
native: all

aarch64: CROSS := aarch64-linux-gnu-
aarch64: all

install-native: $(TARGET)
	install -d $(INSTALL_DIR)
	install -m 755 $(TARGET) $(INSTALL_DIR)/$(TARGET)
	@echo "Installed to $(INSTALL_DIR)/$(TARGET)"
	@echo "Add section entry: see packaging/emumanager"

install-section:
	install -d /storage/iux/sections/applications
	install -m 644 $(SECTION_SRC) /storage/iux/sections/applications/emumanager
	@echo "Section entry installed."

clean:
	rm -f $(TARGET) $(OBJ)
