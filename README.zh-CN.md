# Sarasa's Chip Workshop 照片查看器

> [!IMPORTANT]
> SCW 照片查看器还在重构中。在这条信息被移除之前请不要使用它。

跨平台的、经典 Windows 照片查看器样式的照片查看器。

[![](https://github.com/SarasasChipWorkshop/sarasacw-picture/actions/workflows/windows.yml/badge.svg)](https://github.com/SarasasChipWorkshop/sarasacw-picture/actions/workflows/windows.yml)
[![](https://github.com/SarasasChipWorkshop/sarasacw-picture/actions/workflows/macos.yml/badge.svg)](https://github.com/SarasasChipWorkshop/sarasacw-picture/actions/workflows/macos.yml)
[![](https://github.com/SarasasChipWorkshop/sarasacw-picture/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/SarasasChipWorkshop/sarasacw-picture/actions/workflows/ubuntu.yml)

![SCW 照片查看器 - 主窗口](https://github.com/user-attachments/assets/4b5b1def-8542-4445-a44d-078cefedcd96)

## 简介

Sarasa's Chip Workshop 照片查看器（简称 SCW 照片查看器）是一个轻量级的图片查看器。支持跨平台。界面风格类似于经典的 Windows 照片查看器。它可以让你快速轻松地浏览 JPEG、PNG、GIF、SVG、PSD、KRA、XCF、TGA、HDR、AVIF 以及其他一些常用的图片格式，且无需改变你长期的 Windows 使用习惯。

SCW 照片查看器是[菠萝看图](https://github.com/BLumia/pineapple-pictures)的**硬分叉**，该分叉自菠萝看图 1.4.1 版本开始。

## 安装

从本仓库作者维护的地方下载：

- [GitHub Release 页面](https://github.com/SarasasChipWorkshop/sarasacw-picture/releases)

## 参与贡献

现在已冻结该应用的新功能请求，因为这个应用基本上是经典样式的 Windows 照片查看器的复刻，只是增加了跨平台支持。不过仍然欢迎针对该项目的错误报告和翻译。

## 构建指导

当前状态，我们需要先确保如下依赖可用：

- `cmake`：我们所使用的构建系统
- `python3` 和 [`metaglot`](https://github.com/SarasasChipWorkshop/metaglot)：我们所使用的元数据本地化处理器，推荐使用 `uv tool` 进行隔离式安装（相较于 pip 那种全局式安装）
- 包含 `qt6-svg` 与 `qt6-tools` 组件的 `qt6`：此应用基于 Qt
- `libexiv2`：用以获取和显示更多的图像元信息（可选，推荐）

然后我们就可以使用任何常规的 C++ 编译器如 g++ 或 MSVC 来进行构建了

构建过程就是常规的 CMake 应用构建过程:

``` bash
$ mkdir build && cd build
$ cmake ..
$ cmake --build . # 如果你使用 Makefile 作为 CMake 生成器，也可以直接简单的使用 `make`
```

完毕后，一个名为 `scwpic` 的可执行程序即会被生成以供使用。您也可以选择通过使用 CMake 生成的 `install` 目标继续将其安装到您的设备上。构建步骤完毕后，您也可以使用 `cpack` 来对应用程序进行打包。

当 `exiv2` 在构建时可用时，此项目将尝试使用其进行构建，若您不希望使用 `exiv2`，请传递 `-DEXIV2_METADATA_SUPPORT=OFF` 参数给 `cmake`。此项目在找不到 `exiv2` 时并不会使用 `exiv2`，`EXIV2_METADATA_SUPPORT` 选项可供尽管存在可用的 `exiv2` 但您明确不希望启用其支持时使用。

此应用的图片格式支持依赖于 Qt 的 imageformats 插件，直接从您所用的发行版获取对应的图像格式插件即可。对于 Windows 用户，您可能需要手动构建和使用图像格式插件。下方给出了进一步的说明。

在 Windows、Linux 以及 macOS 系统均可构建此应用，其它有移植 Qt 支持的平台也可能可以进行构建。

## 许可协议

SCW 照片查看器整体使用 MIT 协议进行发布。项目所随的部分源文件可能具备不同但与之兼容的许可协议。
