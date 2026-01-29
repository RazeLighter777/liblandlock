CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -D_GNU_SOURCE

# Normal build uses the vendored kernel headers from ../include
INCLUDES_NORMAL = -I../include -I..

EXAMPLES = abi_version sandbox_readonly header_only_abi_version

all: $(EXAMPLES)

abi_version: abi_version.c ../liblandlock.c ../liblandlock.h
	$(CC) $(CFLAGS) $(INCLUDES_NORMAL) -o $@ abi_version.c ../liblandlock.c

sandbox_readonly: sandbox_readonly.c ../liblandlock.c ../liblandlock.h
	$(CC) $(CFLAGS) $(INCLUDES_NORMAL) -o $@ sandbox_readonly.c ../liblandlock.c

../dist/liblandlock.h:
	$(MAKE) -C .. header-only

header_only_abi_version: header_only_abi_version.c ../dist/liblandlock.h
	$(CC) $(CFLAGS) -I.. -o $@ header_only_abi_version.c

clean:
	rm -f $(EXAMPLES)

.PHONY: all clean
