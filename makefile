# ----------------------------
# Makefile Options
# ----------------------------

NAME = YARPE
ICON = icon.png
DESCRIPTION = "Yet Another Reverse Polish Emulator"
COMPRESSED = NO
ARCHIVED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
