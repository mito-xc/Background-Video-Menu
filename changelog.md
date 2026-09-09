# 2.0.1 - Video Engine and Interactive UI Release

- **Hardware-Accelerated Video Playback**: Integrated Windows Media Foundation (WMF) asynchronous video decoder supporting `.mp4`, `.mov`, `.wmv`, `.webm`, `.avi`, and `.m4v` container formats.
- **Interactive Mouse Framing Viewport**: Added `FramingPopup` with click-and-drag mouse panning, two-layer clipping container, and 16:9 viewport outline.
- **Editable Numeric Zoom Input**: Added direct percentage numeric text input for fast scaling.
- **Card-Based UI Architecture**: Replaced legacy interfaces with native Cocos2d-x and Geode styled card layouts.
- **Player and Particle Suppression**: `Hide Players` now eliminates active particle emitters (CCParticleSystem), trails, and jumping cubes.
- **Multi-Scene Profiles**: Independent scene configuration for Main Menu, Shop, Vault, RobTop Levels, Online Browser, and Garage.
- **Hot Reloading**: Live background and setting synchronization across running scenes.
- **Independent Audio**: Option to mute video streams to preserve Geometry Dash soundtrack.
- **Attribution**: Built upon the original `Menu-Background` codebase by **lil2kki**.

---

# 1.1.7
- Rebuild for latest changes in gd-imgui-cocos.
- Avoid calling clipboard::read on setup.
- Revise README for clarity and conciseness.
- Update version and community link in mod.json.
- Update Geode version to 5.7.1.

# 1.1.3 ... 1.1.6
- Geode v5 migration.

# 1.1.2
- Avoid std::filesystem::path::string usage.

# 1.1.1
- Added warning logs for invalid file selections.

# 1.1.0
- HIDE_GROUND now hides ground elements in other layers.
- Used geode::utils::string::pathToString.
- Updated default background asset.
- Setup mode updated.
- Fixed scrolling behavior.
- SDK 4.8.0.

# 1.0.5
- Removed asynchronous gif parsing.
- Background file preload and caching.
- Moved gif loader to user95401.gif-sprites.

# 1.0.0
- Initial release.
