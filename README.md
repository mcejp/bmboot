# 🐝 Bmboot: a loader & monitor for bare-metal code for Zynq UltraScale+ MPSoC

Documentation: https://bmboot.docs.cern.ch/

Technical report: https://edms.cern.ch/document/3028102

### Building docs

```
doxygen Doxyfile; and python3 -m sphinx_autobuild doc doc/_build
```

### Versioning

This project uses semantic versioning: while the version is 0.x, any release can contain breaking changes,
but from 1.0 onwards, a breaking change should be accompanied by a bump of the major version number.

To release a new version:

1. Update CHANGELOG.md & UPGRADING.md on master to show the version number and release date
   ([example here](https://gitlab.cern.ch/be-cem-edl/diot/zynqmp/bmboot/-/commit/a41dfb8ce842c546e87f65db8ce68e9273184e9d))
2. Push to master to ensure no conflicts
3. Tag the commit in `vX.Y` format (e.g., `v1.0`) and push the tag
