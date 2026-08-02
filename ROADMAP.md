# TGXngine — Version 0.2 Roadmap

This document outlines the planned development targets for TGXngine Version 0.2. These features focus on expanding gameplay capabilities, improving engine architecture, and preparing the engine for larger-scale RTS implementations.

## Planned Features

### Consolidate `gameItems` and `world.items` Vectors

Synchronize the application `gameItems` and `world.items` containers into a unified update workflow. Both vectors must execute matching priority sorting sequences to prevent indexing desynchronization during runtime update loops.

Goals:

* Maintain deterministic update ordering.
* Eliminate duplicate sorting logic.
* Reduce synchronization bugs between application and world layers.
* Improve maintainability of the simulation pipeline.

---

### Implement Aircraft Module (`modules/Aircraft/`)

Introduce aviation gameplay systems that allow unit items to operate above the terrain layer.

Planned functionality:

* Flight altitude management.
* Ground-to-air separation logic.
* Specialized aviation state flows:

  * Taxiing
  * Takeoff
  * Patrolling
  * Return to Base (RTB)
* Fuel tracking systems.
* Payload and ammunition management.
* Airfield integration.

---

### Implement Ships Module (`modules/Ships/`)

Expand battlefield environments to support naval operations.

Planned functionality:

* Dedicated naval movement grid.
* Naval pathfinding systems.
* Ship-specific collision handling.
* Water-only movement restrictions.
* Naval combat integration.
* Transport and support vessel foundations.

Ship unit items move exclusively within the naval grid.

---

### UI Improvements

Improve the UI system to support additional layout functionality.

Planned functionality:

* Button anchors.
* Text justification.

---

### Texture Atlas Support for Unit Items

Replace the current single image, single frame implementation with texture atlas support for unit items.

Planned functionality:

* Texture atlas loading.
* Multiple animation frames per unit item.
* Frame selection and playback.

---

### Implement Modular Strategy AI (`modules/AI/`)

Develop a fully modular artificial intelligence framework driven by external data definitions.

Planned functionality:

* Polymorphic state-based AI architecture.
* Dynamic role assignment systems.
* Strategic planning layers.
* Build-order execution systems.
* Economic management behaviours.
* Technology progression systems.
* JSON-driven configuration and behavior loading.
* Support for multiple AI personalities and faction-specific strategies.

---

## Future Scope (Not Planned Yet)

The following features are considered long-term objectives and are not currently scheduled for Version 0.2.

### Multiplayer Networking Layer

Integrate multiplayer networking architectures using the embedded IXWebSocket library.

Long-term objectives:

* Deterministic simulation synchronization.
* Client/server architecture support.
* Peer-to-peer experimentation.
* State replication and rollback investigation.
* Remote command processing.
* Multiplayer lobby infrastructure.

Networking synchronization loops will ultimately integrate directly with the deterministic `world.items` processing sequence to manage state updates across remote multiplayer clients.

---

### Concurrency Systems

Investigate concurrency architectures to improve simulation scalability and performance on modern multi-core processors.

Long-term objectives:

* Parallel processing of simulation workloads.
* Multi-threaded task execution.
* Job system evaluation.
* Performance profiling and workload distribution.
* Thread-safe engine subsystem design.

---

## Version Goals

Version 0.2 aims to establish the foundational systems required for:

* Multi-domain warfare (Land, Air, and Sea).
* Strategic battlefield visibility.
* Data-driven artificial intelligence.
* Improved simulation stability.
* UI improvements.
* Texture atlas support for unit items.
* Future multiplayer integration.

These systems provide the architectural groundwork for future RTS gameplay expansion while preserving TGXngine's modular and data-driven design philosophy.
