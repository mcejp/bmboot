# Bmboot Upgrading Guide

This guide will list any breaking changes between versions. For a list of all changes, see CHANGELOG.md.

## From 0.3

- `Domain::loadAndStartPayload` now requires and additional argument, which can be retrieved by the payload.
  If this functionality is not being used, an arbitrary value can be provided.

## From 0.2 to 0.3

- The FGC4 memory map has been adjusted. This should normally not affect the user, but requires all code to be rebuilt.
- The payload runtime function `startPeriodicInterrupt(period, handler)` has been split into
  `setupPeriodicInterrupt(period, handler)` and `startPeriodicInterrupt`.

## From 0.1 to 0.2

- The function `configureAndEnableInterrupt` has been removed.
  The same effect can be achieved by a call to `setupInterruptHandling` followed by `enableInterruptHandling`.

## From Unversioned to 0.1

- In CMake, payloads should now be declared using the `add_bmboot_payload` function. This ensures that the payload
  is linked for all supported CPUs and it will automatically add the correct runtime and linker options.
- Names of most `bmctl` commands have been changed
- The `bmctl run` command has been added
