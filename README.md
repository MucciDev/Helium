# Helium 🎈

Helium is a high-performance core utility for Geometry Dash that (tries to) eliminates engine-level bottlenecks and redundant CPU calculations without compromising game stability.

## Features

* **Zero-Allocation Logic:** Rewrites Cocos2d-x string formatting and file I/O to use stack memory, bypassing the slow system heap.
* **Intelligent Culling:** Automatically skips draw calls for invisible sprites and physics updates for inactive particle systems.
* **Offscreen Culling:** Skips draw and transform work for objects outside the viewport to reduce per-frame CPU overhead.
* **State-Change Optimization:** Prevents the CPU from recalculating math for objects, labels, and shaders that haven't changed.
* **Hyper-Boot:** Unlocks the engine framerate during startup and defers VRAM cleanup until the menu is reached.
* **Loading Screen Skip:** Optionally hides the loading screen while still keeping boot optimizations active.
* **Hardware Priority:** Configures the Windows scheduler to prioritize the game process for maximum resource access.
* **Memory Anti-Fragmentation:** Forces high initial capacity for batch nodes to prevent lag spikes during level loading.
* **Background Throttling:** Automatically drops the game's framerate when minimized (Alt-Tab) to conserve host machine resources.

* **Benchmarks:** See [benchmark.md](benchmark.md) for a comparison (mod ON vs OFF) including system specs.

## Technical Improvements

* **Memory:** Massive reduction in heap allocations during UI rendering.
* **Rendering:** Lower CPU overhead on object-heavy levels via redundant state culling.
