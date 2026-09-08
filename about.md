# Background Video Menu

High-performance video and image background player with hardware acceleration, interactive mouse framing, and multi-scene customization for Geometry Dash.

### Credits and Attribution
- **Original Base Mod**: [lil2kki/Menu-Background](https://github.com/lil2kki/Menu-Background) by **lil2kki** (user95401).
- **Video Subsystem and UI Architecture**: **mito-xc** & **lil2kki**.

---

### Features
- **Hardware-Accelerated Video Decoding**: Decodes `.mp4`, `.mov`, `.wmv`, `.webm`, `.avi`, and `.m4v` media files asynchronously using Windows Media Foundation (IMFSourceReader) and DirectX Video Acceleration (DXVA) with zero frame drops.
- **Image and Animated Format Support**: Supports standard `.png`, `.jpg`, `.jpeg`, `.gif`, `.webp`, `.jxl`, and `.qoi` images.
- **Interactive Mouse Framing Viewport**: Click Adjust to pan, offset, and frame backgrounds directly within a 16:9 interactive viewport using the mouse.
- **Direct Numeric Zoom Input**: Type the desired zoom percentage directly or adjust incrementally via plus and minus buttons.
- **Independent Audio Management**: Mute video audio to preserve normal Geometry Dash soundtrack and sound effects.
- **Multi-Scene Profiles**: Configure separate backgrounds and options for Main Menu, Shop, Vault, RobTop Levels, Online Browser, and Garage.
- **Pure Clean Mode**: Eliminates player icons, trails, active particle systems (CCParticleSystem), and default dark gradient overlays.
- **Instant Hot Reloading**: Modifications take effect immediately in real-time without restarting the game.
