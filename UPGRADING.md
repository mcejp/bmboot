# Bmboot Upgrading Guide

This guide will list any breaking changes between versions. For a list of all changes, see CHANGELOG.md.

## From Unversioned to 0.1

- In CMake, payloads should now be declared using the `add_bmboot_payload` function. This ensures that the payload
  is linked for all supported CPUs and it will automatically add the correct runtime and linker options.
- Names of most `bmctl` commands have been changed
- The `bmctl run` command has been added
