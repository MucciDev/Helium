# Changelog

## [v2.1.1]
### Fixed
* **Phantom Revisions (REV +1 Bug):** Fixed a critical save-data bug where the engine would automatically increment the revision counter on created levels.
### Removed
* **Deferred Level Loading:** Completely removed the `LocalLevelManager` delayed boot hook to ensure save data integrity and prevent the REV bug.

## [v2.1.0]
### Fixed
* **Reset Icons Bug:** Fixed an issue where the player's custom icon, spider, and robot joints would fail to assemble and reset to default colors after playing a custom level.
* **Menu Log Spam:** Fixed an issue where the boot timer and VRAM garbage collection would trigger every time the player returned to the main menu.
### Removed
* **Global Z-Order Suspension:** Removed `CCNode` transform caching and Z-order overrides as they were responsible for breaking the player object rendering.

## [v2.0.0]
### Added
* **GPU Draw Call Culling:** The engine now automatically skips draw calls for sprites with 0% opacity or invisible states, saving massive GPU/CPU overhead on trigger-heavy levels.
* **VRAM Garbage Collection:** Useless boot textures are now forcibly dumped from VRAM upon reaching the main menu.
### Changed
* **Zero-Allocation Strings:** Upgraded the fast string formatter and I/O cache to use 4KB stack buffers and string views, completely bypassing the slow system heap for 99.9% of text generations.
* **PC-Only Architecture:** Reconfigured CMake optimizations to heavily favor Windows MSVC (`/O2`, `AVX2`, `/fp:fast`) and dropped conflicting cross-platform linker flags.

## [v1.1.0]
### Added
* **Particle Physics Culling:** In-game particle systems (`CCParticleSystemQuad`) now completely skip physics calculations if their opacity is 0% or they are hidden.
* **Geometry Guard:** `CCLabelBMFont` now checks if text has actually changed before destroying and rebuilding its OpenGL vertex arrays.
* **Shader Caching:** Prevents the game from redundantly recompiling shaders when switching layers.

## [v1.0.0] - Initial Release
### Added
* **Fast-Format:** Replaced the legacy Cocos2d-x string formatter with a high-speed C++ implementation.
* **I/O File Caching:** Eliminated "disk-fighting" between mods by caching resolved file paths locally.
* **Hyper-Boot:** Unlocked the engine framerate to 0 during the boot sequence to accelerate startup routines.
* **OS Hijack:** Configured the Windows scheduler to force "High" priority for the Geometry Dash process.
