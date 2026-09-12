# TGXngine — Roadmap

This document tracks TGXngine's development targets: what has shipped, what was planned and remains outstanding, and what the next version sets out to build. Items that were scheduled for a version but not completed are carried forward rather than dropped, so the outstanding work stays visible.

## Delivered

### Version 0.2 — Multi-Domain Warfare and Data-Driven AI

* Aircrafts module (`modules/Aircrafts/`) — flight altitude management, ground-to-air separation, takeoff and landing flows, taxiway claiming and airport integration.
* Ships module (`modules/Ships/`) — dedicated naval movement grid, naval A\* pathfinding, ship-specific collision handling, water-only movement restriction and naval combat.
* FogOfWar module (`modules/FogOfWar/`) — strategic battlefield visibility.
* Turrets module (`modules/Turrets/`) — defensive structures.
* Modular strategy AI (`modules/AI/`) — polymorphic `AIState` architecture, dynamic role assignment against the loaded module set, strategic planning layers, build-order execution and JSON-driven technology progression.
* Skirmish mode — a lobby scene configuring map, per-slot teams and roles, and spectating, writing its result into `SkirmishSetup`.
* Button anchors — interface elements resolve against an anchor point rather than a fixed corner.

### Version 0.3 — The In-Engine Shell

* Shell module (`modules/Shell/`) — a lexer, recursive-descent parser, AST and tree-walking interpreter; an in-memory filesystem; a terminal command set with remote computers; and a code editor with syntax highlighting. Persists to `Resources/shell.json`.
* UI module (`modules/UI/`) — a data-driven portal of screens, windows and pages declared in `portal.json`, with layout expressions that survive any view size.
* The shell in a match — the console runs the real interpreter inside a portal raised over live play, brokered by the `Game` scene so neither module depends on the other.
* AI economic management — the commander spends from the team's shared `EconomyInstance` rather than a private figure, under army and wave limits, and publishes an `AIDebugSnapshot` to an on-screen readout.

---

## Version 0.4 — Multiplayer Networking

Version 0.4 promotes networking from long-term scope to the primary development target. IXWebSocket is already vendored; no networking code exists in the engine yet, so this version builds the layer from the transport up.

The reference implementation is the War of Salvation server, which runs the original RTS over deterministic lockstep. Its architecture sets the shape of this work.

### Deterministic Lockstep Simulation

Every player action becomes a command stamped with an execution tick and broadcast to all clients, which apply it at that tick. Simulation state is never transmitted — only input.

Planned functionality:

* Command stamping and a server tick clock.
* A command queue applied in arrival order at its stamped tick.
* Client pacing that runs a fixed buffer behind the last acknowledged server tick.
* Guarantees that no stamped command can arrive in the past.
* Desynchronization detection through periodic state digests.

The consequence worth stating plainly is that a late reply is indistinguishable from a player with high latency. Nothing arriving over a socket may touch simulation state directly; it becomes a command or it does not happen.

### Client/Server Architecture

Planned functionality:

* A dedicated server process driving the authoritative tick.
* Room and match lifecycle management.
* Remote command processing and broadcast.
* Reconnection and late-join handling.
* Optional SSL transport.

### Multiplayer Lobby

Extend the existing skirmish lobby to network play: room listing, joining, per-slot assignment across connected players, readiness and match start. `SkirmishSetup` already carries a configured match into the game scene and is the natural handover point.

### Networked AI

An AI commander need not run on the machine it plays from. Because decisions enter as ordinary commands, a remote AI cannot desynchronize the simulation regardless of how long it takes to answer or how non-deterministic it is. `modules/AI/` should be reachable over the same command path as a human player.

### Prerequisite

Deterministic lockstep requires a deterministic update order. The `gameItems` and `world.items` consolidation carried forward below is a prerequisite for this version, not an optional cleanup: two containers kept in step by two separately maintained sort comparators will eventually diverge, and under lockstep a divergence on one client is a desynchronized match rather than a local glitch.

---

## Carried Forward

The following were planned for earlier versions and remain outstanding.

### Consolidate `gameItems` and `world.items` Vectors

Synchronize the application `gameItems` and `world.items` containers into a unified update workflow.

Current state: the two vectors are kept aligned by index, and correctness depends on two separate sort calls using matching priority comparators. The duplicate sorting logic the original goal set out to remove is still present.

Goals:

* Maintain deterministic update ordering.
* Eliminate duplicate sorting logic.
* Reduce synchronization bugs between application and world layers.
* Improve maintainability of the simulation pipeline.

---

### Aircraft Flight Systems

The Aircrafts module ships with takeoff, landing, taxiway and airport integration. The remaining aviation systems were planned for 0.2 and are outstanding.

Planned functionality:

* Patrolling state flow.
* Return to Base (RTB) state flow.
* Fuel tracking systems.
* Payload and ammunition management.

---

### Texture Atlas Support for Unit Items

Replace the current single image, single frame implementation with texture atlas support for unit items. Not started; unit items still load one image per frame.

Planned functionality:

* Texture atlas loading.
* Multiple animation frames per unit item.
* Frame selection and playback.

---

### Text Justification

Element anchoring landed with the interface layer, but wrapped body text inside a window is drawn left-aligned only.

Planned functionality:

* Justification for wrapped multi-line text.
* Centred and right-aligned body text within a panel.

---

### Ship Transport and Support Vessels

Naval combat and the carrier are implemented. Troop transport and the broader support vessel foundations remain outstanding.

---

## Future Scope (Not Planned Yet)

### Concurrency Systems

Investigate concurrency architectures to improve simulation scalability and performance on modern multi-core processors.

Long-term objectives:

* Parallel processing of simulation workloads.
* Multi-threaded task execution.
* Job system evaluation.
* Performance profiling and workload distribution.
* Thread-safe engine subsystem design.

The Shell module's `TaskPool` is the engine's first multi-threaded workload and a useful proving ground for thread-safe subsystem design.

---

## Version Goals

Version 0.4 aims to establish:

* Deterministic lockstep simulation across remote clients.
* Client/server match hosting.
* Networked lobby infrastructure.
* AI commanders reachable over the command path.
* A single deterministic item update sequence underpinning all of the above.

These systems extend TGXngine from a single-machine engine to a networked one while preserving its modular and data-driven design philosophy.
