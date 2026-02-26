# Helium 🎈

Helium is a high-performance core utility for Geometry Dash that (tries to) eliminates engine-level bottlenecks and redundant CPU calculations without compromising game stability.

## Features

* **Zero-Allocation Logic:** Rewrites Cocos2d-x string formatting and file I/O to use stack memory, bypassing the slow system heap.
* **Intelligent Culling:** Automatically skips draw calls for invisible sprites and physics updates for inactive particle systems.
* **State-Change Optimization:** Prevents the CPU from recalculating math for objects, labels, and shaders that haven't changed.
* **Hyper-Boot:** Unlocks the engine framerate during startup and defers VRAM cleanup until the menu is reached.
* **Hardware Priority:** Configures the Windows scheduler to prioritize the game process for maximum resource access.

## Technical Improvements
* **Memory:** Massive reduction in heap allocations during UI rendering.
* **Rendering:** Lower CPU overhead on object-heavy levels via redundant state culling.
