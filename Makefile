CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -fPIC
LDFLAGS ?= -shared

# Prefer vendored kernel UAPI headers under ./include
CFLAGS += -Iinclude

LIB_NAME = liblandlock.so
SRC = liblandlock.c
OBJ = $(SRC:.c=.o)
TEST_SRC = tests/test_liblandlock.c
TEST_BIN = tests/test_liblandlock
TEST_BIN_HEADER_ONLY = tests/test_liblandlock_header_only

DIST_DIR = dist
HEADER_ONLY = $(DIST_DIR)/liblandlock.h

all: $(LIB_NAME) $(HEADER_ONLY)

$(LIB_NAME): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c liblandlock.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(TEST_BIN): $(TEST_SRC) liblandlock.c liblandlock.h
	$(CC) $(CFLAGS) -o $@ $(TEST_SRC) liblandlock.c

$(TEST_BIN_HEADER_ONLY): $(TEST_SRC) $(HEADER_ONLY)
	$(CC) $(CFLAGS) -o $@ $(TEST_SRC) -DLL_TEST_HEADER_ONLY

test: $(TEST_BIN) $(TEST_BIN_HEADER_ONLY)
	./$(TEST_BIN)
	./$(TEST_BIN_HEADER_ONLY)

$(DIST_DIR):
	mkdir -p $@

$(HEADER_ONLY): liblandlock.h liblandlock.c include/linux/landlock.h | $(DIST_DIR)
	{ \
		echo '#pragma once'; \
		echo; \
		echo '/* --- Begin vendored Landlock UAPI header: include/linux/landlock.h --- */'; \
		cat include/linux/landlock.h; \
		echo '/* --- End vendored Landlock UAPI header --- */'; \
		echo; \
		sed -e '/^#pragma once[[:space:]]*$$/d' \
		    -e '/^#include "linux\/landlock\.h"[[:space:]]*$$/d' \
		    liblandlock.h; \
		echo; \
		echo '/* Implementation: define LIBLANDLOCK_IMPLEMENTATION in exactly one translation unit. */'; \
		echo '#ifdef LIBLANDLOCK_IMPLEMENTATION'; \
		sed -e '/^#include "liblandlock\.h"[[:space:]]*$$/d' liblandlock.c; \
		echo; \
		echo '#endif /* LIBLANDLOCK_IMPLEMENTATION */'; \
	} > $@

clean:
	rm -f $(OBJ) $(LIB_NAME) $(TEST_BIN) $(TEST_BIN_HEADER_ONLY) $(HEADER_ONLY)

.PHONY: all clean test header-only

header-only: $(HEADER_ONLY)
