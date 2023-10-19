# Bmboot Upgrading Guide

This guide will list any breaking changes and other notes for upgrading from one version to the next.

## From Unversioned to Unreleased

- In CMake, payloads should now be declared using the `add_bmboot_payload` function. This ensures that the payload
  is linked for all supported CPUs and it will automatically add the correct runtime and linker options.
- Names of most `bmctl` commands have been changed
- The `bmctl run` command has been added