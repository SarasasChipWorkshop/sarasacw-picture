# Sarasa's Chip Workshop Photo Viewer

> [!IMPORTANT]
> SCW Photo Viewer is still in refactoring. Please do not use it until this message removes.

Yet another legacy Windows Photo Viewer with cross-platform support.

[![](https://github.com/SarasasChipWorkshop/sarasacw-picture/actions/workflows/windows.yml/badge.svg)](https://github.com/SarasasChipWorkshop/sarasacw-picture/actions/workflows/windows.yml)
[![](https://github.com/SarasasChipWorkshop/sarasacw-picture/actions/workflows/macos.yml/badge.svg)](https://github.com/SarasasChipWorkshop/sarasacw-picture/actions/workflows/macos.yml)
[![](https://github.com/SarasasChipWorkshop/sarasacw-picture/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/SarasasChipWorkshop/sarasacw-picture/actions/workflows/ubuntu.yml)

![SCW Photo Viewer - Main Window](https://github.com/user-attachments/assets/4b5b1def-8542-4445-a44d-078cefedcd96)

## Summary

Sarasa's Chip Workshop Photo Viewer (abbr. SCW Photo Viewer) is a lightweight image viewer that has cross-platform support and similar user interface about legacy Windows Photo Viewer. It allows you view JPEG, PNG, GIF, SVG, PSD, KRA, XCF, TGA, HDR, AVIF and some other frequently used image formats files quickly and easily without changing your long-term Windows usage experience.

SCW Photo Viewer is the **hard fork** of [Pineapple Pictures](https://github.com/BLumia/pineapple-pictures) since Pineapple Pictures 1.4.1 version.

## Installation

Download from the places maintained by the author of this repository:

- [GitHub Release Page](https://github.com/SarasasChipWorkshop/sarasacw-picture/releases)

## Contribution

The new feature request of this application are freezed now because this application basically is a replica of legacy Windows Photo Viewer with additional cross-platform support. However, bug report and translation are welcome.

## Build Instruction

Current state, we need:

- `cmake`: as the build system.
- `python3` and [`metaglot`](https://github.com/SarasasChipWorkshop/metaglot): as the metadata localization processor. An isolated install via `uv tool` (as opposed to a global install like pip) is recommended.
- `qt6` with `qt6-svg` and `qt6-tools`: since the app is using Qt.
- `libexiv2`: able to display more image metadata. (optional, but recommended)

Then we can build it with any proper C++ compiler like g++ or MSVC.

Building it just requires normal cmake building steps:

```bash
$ mkdir build && cd build
$ cmake ..
$ cmake --build .
```

After that, a `scwpic` executable file will be available to use. You can also optionally install it by using the target `install`. After the build process, you can also use `cpack` to make a package.

The project will try to build with `exiv2` when it's available at build time, if you would like to build the project without `exiv2`, pass `-DEXIV2_METADATA_SUPPORT=OFF` to `cmake`. The project will also not use `exiv2` if it's not found, the `EXIV2_METADATA_SUPPORT` option can be useful if you have `exiv2` but specifically don't want to use it.

Image formats supports rely on Qt's imageformats plugins, just get the plugins you need from your distro's package manager will be fine. For Windows user, you may need build and install the imageformats plugin manually, read the content below.

It's possible to build it under Windows, Linux, macOS, and maybe other desktop platforms that Qt is ported to.

## License

SCW Photo Viewer as a whole is licensed under MIT license. Individual files may have a different, but compatible license.
