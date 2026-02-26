# About Helium

Helium targets the most inefficient bottlenecks in the Geometry Dash engine to reclaim lost performance and reduce CPU overhead.

## How it Works

### Intelligent I/O & Memory
Helium eliminates the "disk-fighting" common in heavily modded setups by caching resolved file paths. It also implements a zero-allocation string formatter that uses stack-allocated buffers, preventing the engine from clogging the system heap with thousands of tiny text allocations.

### Engine Optimizations
* **Fast-Format:** Replaces the legacy Cocos2d-x formatter with a modern, high-speed C++ implementation.
* **Shader Caching:** Prevents redundant OpenGL shader recompiles, ensuring faster transitions between layers.
* **State Culling:** Helium intercepts sprite and particle updates to skip physics and draw calls for objects that are invisible or have 0% opacity.
* **Geometry Guard:** Stops the engine from rebuilding text vertex arrays if the string content hasn't changed.

### Hardware Utilization
During the boot sequence, Helium unlocks the engine framerate to finish initialization routines as fast as your CPU allows. It also sets the process priority to "High" on Windows, ensuring the OS treats Geometry Dash as the primary workload.

## Credits
Helium takes inspiration, integrates and expands upon logic from:
* **Fast Format** by matcool
* **Shader Layer Fix** by cgytrus
* **Path Caching** concepts by the Geode Team
