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

all: $(LIB_NAME)

$(LIB_NAME): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c liblandlock.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(TEST_BIN): $(TEST_SRC) liblandlock.c liblandlock.h
	$(CC) $(CFLAGS) -o $@ $(TEST_SRC) liblandlock.c

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(OBJ) $(LIB_NAME) $(TEST_BIN)

.PHONY: all clean test
