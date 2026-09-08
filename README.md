# Background Video Menu

A high-performance Geometry Dash mod for the Geode framework that enables hardware-accelerated video playback and high-resolution image backgrounds with real-time interactive mouse framing, independent audio control, and per-scene profile management.

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

### Interactive Mouse Framing Viewport
- **Real-Time Drag and Offset**: An interactive 16:9 viewport modal allows click-and-drag panning to position media precisely where desired.
- **Two-Layer Clipping Architecture**: Incorporates `CCClippingNode` to ensure video surfaces remain strictly contained within canvas boundaries at any zoom level (up to 1000%).
- **Visual Viewport Highlight**: Features a high-contrast cyan bounding box representing the exact in-game screen area, with outside regions darkened by a semi-transparent peripheral mask.
- **Direct Numeric Zoom Input**: Zoom scale can be adjusted via direct percentage typing (`100%`, `150%`, `80%`) or incremental `+` / `-` buttons.

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

### UI and Particle Cleaner (Pure Clean Mode)
- **Player and Particle Suppression**: When enabled, suppresses player icons, motion trails, and active particle emitters (`CCParticleSystem` / `CCParticleSystemQuad`) from the main menu.
- **De-cluttering**: Hides RobTop default background tiles, gradients, and dark tint layers for an unobstructed full-screen background display.
- **Transparent Level Cards**: Optional transparency for RobTop and online level browser cards.

### Hot Reloading
- Configuration updates and profile adjustments take effect instantly in real-time without requiring game restarts or layer reloading.

---

## Technical Specifications and Official References

- **Microsoft Windows Media Foundation API Reference**:  
  https://learn.microsoft.com/en-us/windows/win32/medfound/media-foundation-programming-reference
- **Microsoft IMFSourceReader Interface Documentation**:  
  https://learn.microsoft.com/en-us/windows/win32/api/mfreadwrite/nn-mfreadwrite-imfsourcereader
- **Khronos OpenGL Specification (glTexSubImage2D)**:  
  https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexSubImage2D.xhtml
- **Geode SDK Documentation and Developer Guide**:  
  https://docs.geode-sdk.org/
- **Cocos2d-x Engine Documentation**:  
  https://docs.cocos2d-x.org/

---

## Building from Source

### Prerequisites
- [Geode CLI](https://docs.geode-sdk.org/getting-started/tools) (v3.8.0 or higher)
- Geode SDK (v5.10.1)
- Microsoft Visual Studio 2022 Build Tools (MSVC v143, C++23 standard)
- Geometry Dash 2.2081

### Build Command (PowerShell)
```powershell
geode build
```

The resulting `.geode` package will be generated inside the `build/` directory and automatically deployed to your configured Geometry Dash installation.

---

## License

This project is licensed under the original terms provided in the [LICENSE.txt](LICENSE.txt) file.
