# Background Video Menu

A high-performance Geometry Dash mod for the Geode framework that enables hardware-accelerated video playback and high-resolution image backgrounds with real-time interactive mouse framing, independent audio control, and per-scene profile management.

Weeeeeell, I wanted to take on this project on a bit of a whim; I found a very well-made version and decided to use it as a starting point,
which made this possible. There are some minor errors and bugs on mobile devices—partly due to how resource-heavy it is—but I’ll be addressing those later on bye bye
---

## Credits and Attribution

This project is built upon the foundational work of the open-source mod [Menu-Background](https://github.com/lil2kki/Menu-Background) created by **lil2kki** (user95401).

- **Original Base Repository**: [lil2kki/Menu-Background](https://github.com/lil2kki/Menu-Background)
- **Video Engine Architecture and Interactive UI Implementation**: [mito-xc](https://github.com/mito-xc) & lil2kki.

---

## Technical Overview and Architecture

Standard animated backgrounds implemented through sequence frames or naive software decoding often suffer from significant frame drops, high memory overhead, and main-thread CPU bottlenecks. 

**Background Video Menu** addresses these limitations by separating media decoding from the game's rendering thread and utilizing operating system level hardware decoding pipelines.

### 1. Asynchronous Hardware Decoding via Windows Media Foundation
The video subsystem utilizes the native Windows Media Foundation (WMF) API:
- **IMFSourceReader Pipeline**: Video streams are decoded asynchronously on a dedicated worker thread via [`IMFSourceReader`](https://learn.microsoft.com/en-us/windows/win32/api/mfreadwrite/nn-mfreadwrite-imfsourcereader). This prevents media I/O and stream parsing from blocking the main Cocos2d game loop.
- **Hardware Acceleration (DXVA / MFT)**: Video frames are decoded using hardware-accelerated Media Foundation Transforms (MFTs) supporting modern container and codec specifications, including H.264/AVC, H.265/HEVC, VP9, and Windows Media Video. Official documentation: [Microsoft Media Foundation Architecture](https://learn.microsoft.com/en-us/windows/win32/medfound/microsoft-media-foundation-architecture).
- **Double-Buffered Lockless Frame Queue**: Decoded raw pixel buffers (`RGB32` / `RGBA8888`) are synchronized through atomic thread-safe buffers, ensuring zero stutter during continuous playback loops.

### 2. High-Throughput OpenGL Texture Streaming
Instead of reallocating OpenGL texture memory on every frame, the video engine preallocates a persistent GPU texture and streams updated pixel data:
- **Zero-Reallocation Updates via `glTexSubImage2D`**: Decoded video frames are directly mapped to active texture memory using [`glTexSubImage2D`](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexSubImage2D.xhtml). This minimizes GPU driver overhead and eliminates memory fragmentation.
- **Aspect Ratio Transformations**: Texture coordinates and quad vertices are calculated dynamically on the GPU to support `Cover`, `Contain`, `Stretch`, and `Center` scaling modes without distorting pixel aspect ratios.

### 3. Native Cocos2d-x Scene Graph Integration
- Implemented natively within the Cocos2d-x scene hierarchy (`CCNode` / `CCSprite`), ensuring full compatibility with Geometry Dash's rendering pipeline. Official framework reference: [Cocos2d-x Engine Architecture](https://docs.cocos2d-x.org/).
- Modding hooks and lifecycle management are provided by the [Geode SDK](https://docs.geode-sdk.org/).

---

## Features

### Supported Media Formats
- **Video Containers**: `.mp4`, `.mov`, `.wmv`, `.webm`, `.avi`, `.m4v`
- **Image Formats**: `.png`, `.jpg`, `.jpeg`, `.gif`, `.webp`, `.jxl`, `.qoi`

### Independent Audio Management
- **Mute Video Audio**: Retains standard Geometry Dash in-game music and sound effects while video backgrounds play silently.
- **Seamless Loop Control**: Frame timestamps automatically reset upon reaching stream duration for seamless continuous loops.

### Multi-Scene Profile Support
Independent configurations (media source, offset, zoom, opacity, and display modes) can be assigned individually to:
- Main Menu (`MenuGameLayer`)
- Shop (`GJShopLayer`)
- The Vault / Secret Rewards (`SecretRewardsLayer`)
- RobTop Official Level Select (`LevelSelectLayer`)
- Online Level Browser (`LevelBrowserLayer`)
- Garage / Icon Customization (`GJGarageLayer`)

---

## Building from Source

### Prerequisites
- [Geode CLI](https://docs.geode-sdk.org/getting-started/tools) (v3.8.0 or higher)
- Geode SDK (v5.10.1)
- Microsoft Visual Studio 2022 Build Tools (MSVC v143, C++23 standard)
- Geometry Dash 2.2081 (for now)

---

## License

This project is licensed under the original terms provided in the [LICENSE.txt](LICENSE.txt) file.
