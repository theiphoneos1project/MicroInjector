# MicroInjector
A powerful runtime code injection platform designed specifically for iPhone OS 1, targeted for armv6 devices.

Tested on:
- iPhone OS 1.0 on an iPhone 1st generation
- iPhone OS 1.1 on an iPod touch 1st generation
- iPhone OS 1.1.5 on an iPod touch 1st generation

The public-facing header with native functions is located at [MicroInjector.h](MicroInjector.h). Shims for Cydia Substrate are implemented in [SubstrateShims.c](SubstrateShims.c).

### How to install
Head over to the [Releases](<TBD/LINK>) section and download the `.PXL`. Install it with iBrickr using Windows XP.

### How to compile manually
Make sure you are in an environment with `clang` and have assembled an unofficial SDK for iPhone OS 1. Also, make sure you have the patched `ld` with the Objective-C fragile runtime support. Feel free to modify the [`Makefile`](./Makefile) as needed for your paths.

Clone the repo and the run this to compile the binaries:
```bash
make clean && make
```
To package the `.PXL` file, run:
```bash
make pxl
```

#### License
This project is licensed under [MIT](LICENSE).

###### Copyright (c) 2026 Nightwind