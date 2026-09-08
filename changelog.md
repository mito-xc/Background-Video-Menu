# 2.0.0 - The Video Engine & Interactive UI Update

- **Hardware-Accelerated Video Playback**: Added Windows Media Foundation (WMF) asynchronous video decoder supporting `.mp4`, `.mov`, `.wmv`, `.webm`, `.avi`, `.m4v`.
- **Interactive Mouse Framing Viewport**: Introduced `FramingPopup` allowing real-time mouse dragging to position and frame backgrounds with 16:9 preview.
- **Editable Zoom Input**: Added direct `%` numeric text input for fast zoom adjustments.
- **Card-Based UI Redesign**: Compact, clean native Cocos2d/Geode popup replacing legacy UI.
- **Complete Player & Particle Cleaning**: `Hide Players` now eliminates all player icons, trail particles, and jump particle emitters.
- **Multi-Scene Support**: Independent background profiles for Main Menu, Shop, Vault, RobTop Levels, Online Browser, and Garage.
- **Live Hot-Reloading**: Background modifications take effect immediately without restarting the game.
- **Independent Audio**: Option to mute video audio to preserve normal GD music.
- **Credits**: Based on the original `Menu-Background` mod by **lil2kki**.

---

# 1.1.7
basicly just rebuild for latest changes in gd-imgui-cocos.

- **[Don't call clipboard::read on setup](https://github.com/matcool/gd-imgui-cocos/commit/7eea023986b46a9077996128d6f63a25af5b4c13)**
- [Revise README for clarity and conciseness](https://github.com/lil2kki/Menu-Background/commit/6b77f17c7f9261147a875926d8baff3d3faeae87)
- [Update version and community link in mod.json](https://github.com/lil2kki/Menu-Background/commit/55e9402da0630119628718743993aee661f5f34f)
- [Update geode version to 5.7.1](https://github.com/lil2kki/Menu-Background/commit/6c973591070c8313f9f1bba39f37faeb72f502d2)

# 1.1.3 ... 1.1.6
v5 migrate.

# 1.1.2
- avoid `std::filesystem::path::string` usage on lines 17, 18, and 19... 

# 1.1.1
- added warns for invalid file selections

# 1.1.0
- `HIDE_GROUND` now hides grounds in other layers
- used `geode::utils::string::pathToString`
- default bg name is `edit_barBG_001.png` now
- setup mode is enabled by default
- setup mode window updated
- fixed scrolling bugs
- logo updated
- SDK 4.8.0

# 1.0.5
- removed async gif parsing
- bg file preload
- gif caching
- fixed bugs
- gif loader moved to [user95401.gif-sprites](https://geode-sdk.org/mods/user95401.gif-sprites)

# 1.0.0
- First release.
