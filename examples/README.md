# Examples

This directory contains small, self-contained examples showing how to use `liblandlock`.

## Build

From the repo root:

- Build all examples:
  - `make -C examples`

- Clean:
  - `make -C examples clean`

## Examples

- `abi_version.c`
  - Queries the running kernel for the supported Landlock ABI version.

- `sandbox_readonly.c`
  - Creates a ruleset that allows read-only access to a temporary directory, then demonstrates that writes are blocked.
  - If the running kernel doesn’t support Landlock (or it’s disabled), it prints a message and exits successfully.

- `header_only_abi_version.c`
  - Same as `abi_version.c`, but uses the generated header-only amalgamation (`dist/liblandlock.h`).
  - This does _not_ require installing Landlock UAPI headers because the vendored header is inlined into the amalgamation.
