# liblandlock

Small C library providing a friendlier API around the Linux Landlock LSM.

## Usage

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

## Build

### Vendored Library (directly including in your build)

- Make sure linux headers are installed.

- Add liblandlock.c/h and include/linux/landlock.h to your project. You may need to adjust the include path here:

```
liblandlock.h
2:#include "linux/landlock.h"
```

### Shared library (`liblandlock.so`)

- Make sure linux headers are installed.

- Build:
  - `make`

This builds `liblandlock.so` from `liblandlock.c`.

Include liblandlock.h in your project.

### Header-only amalgamation (`dist/liblandlock.h`)

- Make sure linux headers are installed.

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

### Header-only style

First, ensure linux headers are installed and are in your include path.

1. Generate the amalgamation:

- `make header-only`

2 Copy the header into your project:

- `cp dist/liblandlock.h <your project>`

2. In exactly one translation unit, define `LIBLANDLOCK_IMPLEMENTATION` before including the file:

```c
#define LIBLANDLOCK_IMPLEMENTATION
#include "liblandlock.h"

/* ...same usage as above... */
```

Build (example):

- `cc -I. -o demo demo.c`

## More examples

See `examples/`:

- `make -C examples`
- `examples/README.md`
