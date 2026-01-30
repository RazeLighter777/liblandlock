# liblandlock

Ergonomic C library for Landlock sandboxing.

## Quick start

```c
#include "liblandlock.h"

#include <stdio.h>

int main(void)
{
  ll_ruleset_attr_t attr = ll_ruleset_attr_defaults();
  attr = ll_ruleset_attr_fs(attr, LL_ACCESS_GROUP_FS_READ);

  ll_ruleset_result_t res = ll_ruleset_create_result(attr);
  if (LL_ERRORED(res.err))
  {
    fprintf(stderr, "ll_ruleset_create failed: %s (%d)\n", ll_error_string(res.err), res.err);
    return 1;
  }

  ll_error_t err = ll_ruleset_add_path_ro(res.ruleset, "/usr");
  if (LL_ERRORED(err))
  {
    fprintf(stderr, "ll_ruleset_add_path failed: %s (%d)\n", ll_error_string(err), err);
    ll_ruleset_close(res.ruleset);
    return 1;
  }

  err = ll_ruleset_enforce(res.ruleset, 0);
  if (LL_ERRORED(err))
  {
    fprintf(stderr, "ll_ruleset_enforce failed: %s (%d)\n", ll_error_string(err), err);
    ll_ruleset_close(res.ruleset);
    return 1;
  }

  ll_ruleset_close(res.ruleset);
  return 0;
}
```

Build (example):

- `cc -Iinclude -o demo demo.c liblandlock.c`

## Requirements

Build requirements (no specific OS or package manager assumed):

- A C toolchain (a C compiler + linker)
- `make`
- Linux headers installed.

Runtime requirements:

- Landlock is a Linux feature. Building the library does not require Landlock support at runtime, but actually applying a sandbox does.
- The test suite will automatically skip sandboxing assertions if the running kernel doesn’t support Landlock or if it is disabled.

## Use this in your project

Choose one of the three integration styles below.

### 1) Header-only amalgamation (`dist/liblandlock.h`) — recommended

This option is a single-header distribution with optional implementation.

1. Build the amalgamated header:

- `make header-only`

2. Copy it into your project:

- `cp dist/liblandlock.h <your project>`

3. In exactly one translation unit, define `LIBLANDLOCK_IMPLEMENTATION` before including the header:

```c
#define LIBLANDLOCK_IMPLEMENTATION
#include "liblandlock.h"

/* ...same usage as above... */
```

Example build:

- `cc -I. -o demo demo.c`

### 2) Vendored sources

This option adds the sources directly to your build.

1. Ensure Linux headers are installed on the build machine.
2. Add the following files to your project:

- liblandlock.c
- liblandlock.h
- include/linux/landlock.h

3. Ensure your include path allows this include in liblandlock.h to resolve:

```
liblandlock.h
2:#include "linux/landlock.h"
```

4. Build and link the library together with your code.

Example:

- `cc -Iinclude -o demo demo.c liblandlock.c`

### 3) Shared library (`liblandlock.so`)

This option builds a shared library and links to it.

1. Ensure Linux headers are installed on the build machine.
2. Build the shared library:

- `make`

3. Include `liblandlock.h` in your project and link with `liblandlock.so`.

## What’s inside the header-only build

The header-only build output contains:

- The vendored Landlock UAPI header inlined (so consumers don’t need to install Landlock headers)
- The public API from `liblandlock.h`
- The implementation from `liblandlock.c` behind `#ifdef LIBLANDLOCK_IMPLEMENTATION`

## Tests

- `make test`

This builds and runs two test binaries:

- A regular build using `liblandlock.c` + `liblandlock.h`
- A header-only build using `dist/liblandlock.h`
- `#pragma once` at the top

## More examples

See `examples/`:

- `make -C examples`
- `examples/README.md`
