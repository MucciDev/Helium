# About Helium

Helium targets the slowest parts of Geometry Dash and the Cocos2d-x engine to reclaim lost performance.

## How it Works

### 🧵 Better Multithreading
Helium stops the game from waiting for large files. Save data is loaded on background threads, and file paths are cached locally so mods don't fight over disk access.

### ⚡ Engine Fixes
* **Fast-Format:** Fixes a bug where the engine wastefuly allocated 100KB of memory for every single text string.
* **Shader Caching:** Keeps shaders in memory so the game doesn't have to recompile them every time you boot.
* **Parallel Loading:** Uses background threads to parse level data, drastically reducing wait times on high-object levels.

### 🖥️ Hardware Power
Helium halves texture bandwidth during startup and forces Windows to prioritize Geometry Dash over background tasks. It culled invisible objects and particles to boost in-game FPS without changing physics.

## Credits
Helium integrates logic from:
* **Level-Load** by emeraldblock
* **Level Shaders Fix** by cgytrus
* **Fast Format** by matcool
