# liblandlock

Small C library providing a friendlier API around the Linux Landlock LSM.

This repo produces two primary “build outputs”:

- A shared library: `liblandlock.so`
- A single-file header-only amalgamation: `dist/liblandlock.h`

The project vendors a Linux UAPI Landlock header under `include/linux/landlock.h` and uses it by default.

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

### Header-only amalgamation (`dist/liblandlock.h`)

- Make sure linux headers are installed.

The header-only build output contains:

- `#pragma once` at the top
- The vendored Landlock UAPI header inlined (so consumers don’t need to install Landlock headers)
- The public API from `liblandlock.h`
- The implementation from `liblandlock.c` behind `#ifdef LIBLANDLOCK_IMPLEMENTATION`

Build it with:

- `make header-only`

## Tests

- `make test`

This builds and runs two test binaries:

- A regular build using `liblandlock.c` + `liblandlock.h`
- A header-only build using `dist/liblandlock.h`

## Usage

### Shared-library style (compile the `.c` directly)

This is the simplest approach if you’re just embedding the code:

```c
#include "liblandlock.h"

#include <stdio.h>

int main(void)
{
    ll_ruleset_attr_t attr = ll_ruleset_attr_make(LL_ABI_LATEST, LL_ABI_COMPAT_BEST_EFFORT);
    (void)ll_ruleset_attr_handle(&attr, LL_RULESET_ACCESS_CLASS_FS,
                                LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_READ_DIR);

    ll_ruleset_t *ruleset = NULL;
    ll_sandbox_result_t result = LL_SANDBOX_RESULT_NOT_SANDBOXED;
    ll_error_t err = ll_ruleset_create(&attr, 0, &result, &ruleset);
    if (err != LL_ERROR_OK)
    {
        fprintf(stderr, "ll_ruleset_create: %s (%d)\n", ll_error_string(err), err);
        return 1;
    }

    /* Example: allow read-only access to /tmp (adjust to your needs). */
    err = ll_ruleset_add_path(ruleset, "/tmp",
                              LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_READ_DIR, 0);
    if (err != LL_ERROR_OK)
    {
        fprintf(stderr, "ll_ruleset_add_path: %s (%d)\n", ll_error_string(err), err);
        ll_ruleset_close(ruleset);
        return 1;
    }

    err = ll_ruleset_enforce(ruleset, 0);
    ll_ruleset_close(ruleset);

    if (err != LL_ERROR_OK)
    {
        fprintf(stderr, "ll_ruleset_enforce: %s (%d)\n", ll_error_string(err), err);
        return 1;
    }

    return 0;
}
```

Build (example):

- `cc -Iinclude -o demo demo.c liblandlock.c`

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
