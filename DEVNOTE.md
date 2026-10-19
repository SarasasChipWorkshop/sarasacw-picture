# Developer Notes

This document describes maintenance workflows and policies of this repository that are easy to get wrong or hard to discover from the source tree alone. It is intended for developers of this repository; see `README.md` for general instructions.

## Bumping the Version

The project version is maintained by hand in only one place, `CMakeLists.txt`. Modify its `project` statement at the top of the file when bumping the version.

Additionally, when releasing a new version of this application, please update all of the following items for a better user experience:

- `NEWS`: add a new section at the very top of the file, following the format of the existing entries.
- `dist/org.sarasaschipworkshop.picture.metainfo.xml.liquid`: add a new `<release>` element at the top of the `<releases>` section.

Finally, mark the release with a git tag named exactly `x.y.z` (no `v` prefix, strictly three numeric components).

## Maintaining Translator Credits

When adding a new translator for an existing language, or a new language, please edit `assets/htmls/special-thanks.html` by hand to show the corresponding credits in the application interface.

If you have added a language for the application, please make sure to also add it for all distribution translations. The available languages of the distribution translations should be kept synchronized with the application translations.

## Qt Version Support Policy

### Policy for CI

- The Qt version installed by the Windows and macOS workflows is pinned to the one used by the hard fork upstream ([Pineapple Pictures](https://github.com/BLumia/pineapple-pictures)).
- The Ubuntu workflow always uses the Qt provided by the package manager of the latest Ubuntu LTS version.

### Policy for the Project

The minimum Qt version that the project itself declares follows the formula `min(A, B)`, where `A` is the lowest Qt version used by any CI workflow and `B` is the Qt version shipped by the latest Debian stable release.

## File Type Associations

The image formats supported by this application are varied. Considering the workload of creating icons, we cannot assign each of them a dedicated icon, so it is inevitable that multiple file extensions share a single icon. We list this relation by packaging channel for developers, and at the same time you can also see the file extensions that each packaging channel wants to associate.

The assignment follows a categorization:

- Photographic and high dynamic range formats use the `jpg` icon.
- Formats that can carry transparency use the `png` icon.
- Bitmap artwork produced by graphics editors uses the `bmp` icon.
- Formats that contain animation use the `gif` icon.
- Vector formats, archaic formats, and icon formats use the `svg` icon.

### MSIX Package

| Cluster Name | Icon | File Extensions |
| --- | --- | --- |
| JPEG | `jpg` | `.jpg`, `.jpeg`, `.jfif` |
| GIF | `gif` | `.gif` |
| PNG | `png` | `.png`, `.apng` |
| SVG | `svg` | `.svg` |
| WebP | `png` | `.webp` |
| AVIF | `png` | `.avif` |
| ICO | `svg` | `.ico` |
| ICNS | `svg` | `.icns` |
| PSD | `bmp` | `.psd`, `.psb`, `.pdd`, `.psdt` |
| KRA | `bmp` | `.kra` |
| XCF | `bmp` | `.xcf` |
| TARGA | `svg` | `.tga` |
| TIFF | `jpg` | `.tif`, `.tiff` |
| BMP | `bmp` | `.bmp` |
| WBMP | `bmp` | `.wbmp` |
| DDS | `bmp` | `.dds` |
| HDR | `jpg` | `.hdr`, `.exr` |
| IFF | `svg` | `.iff` |
| PIC | `svg` | `.pic` |
| PCX | `svg` | `.pcx` |
| QOI | `png` | `.qoi` |
| Sun Raster | `svg` | `.ras`, `.sun` |
| ORA | `bmp` | `.ora` |
| ANI | `gif` | `.ani` |
| PFM | `jpg` | `.pfm`, `.phm` |
| SGI | `svg` | `.bw`, `.rgb`, `.rgba`, `.sgi` |
| PXR | `svg` | `.pxr` |
| SCT | `svg` | `.sct` |

### Inno Setup Installer

| Cluster Name | Icon | File Extensions |
| --- | --- | --- |
| JPEG | `jpg` | `.jpg`, `.jpeg`, `.jfif` |
| GIF | `gif` | `.gif` |
| PNG | `png` | `.png`, `.apng` |
| SVG | `svg` | `.svg` |
| WebP | `png` | `.webp` |
| AVIF | `png` | `.avif` |
| ICO | `svg` | `.ico` |
| ICNS | `svg` | `.icns` |
| PSD | `bmp` | `.psd`, `.psb`, `.pdd`, `.psdt` |
| KRA | `bmp` | `.kra` |
| XCF | `bmp` | `.xcf` |
| TARGA | `svg` | `.tga` |
| TIFF | `jpg` | `.tif`, `.tiff` |
| BMP | `bmp` | `.bmp` |
| WBMP | `bmp` | `.wbmp` |
| DDS | `bmp` | `.dds` |
| HDR | `jpg` | `.hdr`, `.exr` |
| IFF | `svg` | `.iff` |
| PIC | `svg` | `.pic` |
| PCX | `svg` | `.pcx` |
| QOI | `png` | `.qoi` |
| Sun Raster | `svg` | `.ras`, `.sun` |
| ORA | `bmp` | `.ora` |
| ANI | `gif` | `.ani` |
| PFM | `jpg` | `.pfm`, `.phm` |
| SGI | `svg` | `.bw`, `.rgb`, `.rgba`, `.sgi` |
| PXR | `svg` | `.pxr` |
| SCT | `svg` | `.sct` |

### FreeDesktop Desktop Entry

| Cluster Name | Mimetypes |
| --- | --- |
| JPEG | `image/jpeg`, `image/jpg`, `image/jpe`, `image/jpeg24` |
| PNG | `image/png` |
| GIF | `image/gif` |
| SVG | `image/svg`, `image/svg+xml` |
| BMP | `image/bmp`, `image/bmp24` |
| TIFF | `image/tif`, `image/tiff`, `image/tiff24`, `image/ptif` |
| DDS | `image/dds` |
| PCX | `image/pcx` |
| SGI | `image/sgi` |
| JNG | `image/jng` |
| JPEG 2000 | `image/j2k`, `image/jp2` |
| PICT | `image/pct` |
| HD Photo | `image/wdp` |
| camera raw | `image/arw`, `image/dng`, `image/mef` |
| Targa variants | `image/icb`, `image/vda`, `image/vst` |
| XBM | `image/xbm` |
| PhotoCD | `image/pcd` |

### macOS Application Bundle

| Cluster Name | Mimetypes | File Extensions |
| --- | --- | --- |
| JPEG | `image/jpeg` | `.jpg`, `.jpeg`, `.jfif` |
| PNG | `image/png` | `.png` |
| WebP | `image/webp` | `.webp` |
| GIF | `image/gif` | `.gif` |
| SVG | `image/svg+xml` | `.svg` |

## Adding a New File Type

A new file type must be added to every channel listed above in the same change. Specifically, you should:

- Update the tables in this document accordingly.
- Windows RC File
    * Define the display name under the next numeric string key in `dist/sarasacw-picture.rc.in.toml`.
    * Extend the string table ID range in `dist/sarasacw-picture.rc.in.liquid` to cover the newly added string resource.
- MSIX Package
    * Define the display name in `dist/AppxManifest.xml.in.toml`.
    * Add a `uap:FileTypeAssociation` element to `dist/AppxManifest.xml.in.liquid` with the extensions, the display name reference, and the logo reference.
- Inno Setup Installer
    * Edit the `[Registry]` section of `dist/scwpic.iss.in.liquid`.
        - Copy an existing file family block, adjust the ProgId, the referenced string and icon resources, and the extension list.
        - Add the new extensions to the `SupportedTypes` registry key.
- FreeDesktop Desktop Entry
    * Append the mimetype to the `MimeType=` key of `dist/org.sarasaschipworkshop.picture.desktop.liquid` (the type name and icon are resolved by the desktop environment, so nothing else is needed).
- macOS Application Bundle
    * Add a `CFBundleDocumentTypes` entry to `dist/Info.plist.in.liquid` with the extensions, the display name, and the mimetypes.
    * Update `dist/Info.plist.in.toml` and `dist/InfoPlist.strings.liquid` for the display name translation.
