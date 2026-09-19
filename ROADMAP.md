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

### Version 0.3 — The In-Engine Shell

* Shell module (`modules/Shell/`) — a lexer, recursive-descent parser, AST and tree-walking interpreter; an in-memory filesystem; a terminal command set with remote computers; and a code editor with syntax highlighting. Persists to `Resources/shell.json`.
* UI module (`modules/UI/`) — a data-driven portal of screens, windows and pages declared in `portal.json`, with layout expressions that survive any view size. This carries the button anchors planned for 0.2: every element resolves against an anchor point rather than a fixed corner.
* The shell in a match — the console runs the real interpreter inside a portal raised over live play, brokered by the `Game` scene so neither module depends on the other.
* AI economic management — the commander spends from the team's shared `EconomyInstance` rather than a private figure, under army and wave limits, and publishes an `AIDebugSnapshot` to an on-screen readout.
* Unified item update workflow — an `Item` carries its own `ItemInstance` rather than borrowing the one at its index in `world.items`, and both containers are ordered by the single `ItemOrder` comparator. This closes the `gameItems`/`world.items` consolidation carried forward from 0.2.

### Version 0.4 — Multiplayer Networking

* Deterministic lockstep (`src/Net/`) — commands stamped for an execution tick and applied by every client on it, client pacing behind the server's clock, no stamped command arriving in the past, and one seeded stream of chance for the whole match.
* Desynchronization detection — two digests per client: the commands it applied, held against the server's fold, and its world, held against the other clients'.
* TGXngineServer — a dedicated server driving the tick, with room and match lifecycle, reconnection and late join by replay from the seed, and optional TLS.
* Multiplayer lobby — rooms with occupancy and map; seats, sides, maps and readiness per seat; observers as a first-class way in.
* What a player asks for — orders carrying what was under the cursor, placement, and paying for a unit, which makes a team's treasury shared state.
* Remote filesystems — every player's console is a computer the others can `connect` to and work in, over traffic the server relays beside the command path and never into the simulation.
* Processes — a player's buildings are processes, listed by `ps` with their power and stopped or restarted by `kill` and `start` through stamped commands; `kill` also interrupts a runaway program.
* The portal no longer pauses a networked match, and cheats are refused in one.

Carried into 0.5: a world on the server, AI commanders over the command path, and production as shared state.

---

## Version 0.5 — Arena Mode

An arena is a room where AI commanders play one another and people watch. What 0.4 built is a clock and a command path with no world behind them, and an arena needs a world with nobody at a keyboard to supply one. Version 0.5 puts a world on the server, and the commanders beside it.

### Hosting the Modules Headlessly

The whole contract is the five calls in `Simulation.h`: `Load`, `Apply`, `Advance`, `Digest`, `Outcome`. Anything satisfying them can drive a match, provided `Advance` is a pure function of the ticks and commands it has been given and `Digest` folds the same fields in the same order on every machine.

Planned functionality:

* Run the engine's modules without a `Window`, which they all currently require.
* Drive the authoritative tick from a real world rather than from the commands alone.
* Decide a match's outcome server-side.
* Check a client's world fold against the server's own rather than against another client's.

### Networked AI

With a world on the server, a commander can run beside it and enter its decisions as ordinary commands. Nothing about that can desynchronize a match: a remote AI is only ever a player with high latency.

Planned functionality:

* `modules/AI/` reachable over the command path.

### Arena Rooms

The protocol already carries the flag: `join_arena` joins a room and marks it AI versus AI, and `start_game` says so. Nothing yet plays in one.

Planned functionality:

* A room flagged as an arena seats a commander on every side and starts without anybody saying they are ready.
* Spectating as the way in: observers join, watch, and can join late through the replay path 0.4 built.
* A server-decided outcome, reported to everyone watching.

### Per-Team Production

Production is the last thing a player does that is not shared state. The sidebar is the local player's alone, so a match has no model of what another side is building until the unit appears.

Planned functionality:

* A production model per team rather than per sidebar.
* Build queues folded into the digest alongside the treasuries.

---

## Version 0.6 — Access and Hacking

Version 0.4 lets a player into another's filesystem freely and keeps processes to their own console. Version 0.6 puts a lock on the door and makes getting through it worth something: a player who breaks into another's computer can run programs there, and those programs can reach the match.

### Access

Planned functionality:

* Credentials on every player's computer, set from its own console, and checked by the owner before a session opens.
* The `hacker` platform the server already accepts, given a meaning.

### Remote Execution

Planned functionality:

* A program started on another player's computer runs there, on that console's task pool against that filesystem, with its output streamed back to whoever started it.
* `ps` and `kill` on a computer a player has broken into.

### Hacking

A program can reach the match only by asking for a command. It is stamped by the server and applied by every client on the same tick, exactly as an order is, so a hack can change the match without being able to desynchronize it. Nothing a program does is applied where it runs.

Planned functionality:

* A `hack` command kind beside `order`, `event` and `kill`, carrying its effect and the side it targets. The first effect is cutting a side's power.
* Power with consequences. A side without enough, whether its grid is short or a hack has cut it, has its defences go offline, loses its radar and minimap, and builds at a reduced rate rather than stopping. The consequences are simulation state, so they are the same on every client and folded into the digest.
* A cut stays cut until it is restored, by the owner from their own console or by raising a new powerplant.
* Turrets become processes once their behaviour depends on power.
* The server accepts a hack against a side only from that side's own connection, and only while an authorised session into its computer is open. A tampered client could still ignore hacks against itself; closing that needs the headless host from 0.5 to decide them instead.

---

## Carried Forward

The following were planned for earlier versions and remain outstanding.

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

Element anchoring landed with the 0.3 UI module, but the other half of the 0.2 UI item did not: wrapped body text inside a window is drawn left-aligned only.

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

Version 0.5 aims to establish:

* The engine's modules hosted headlessly, so the server drives a world rather than a clock.
* A server-decided outcome, and a client's world checked against the server's rather than against another client's.
* AI commanders reachable over the command path.
* Arena rooms, where commanders play one another and people watch.
* Production as shared state rather than local interface state.

Version 0.6 aims to establish:

* Access control on every player's computer.
* Programs run on another player's computer, with their output returned.
* Hacks that reach the match through the command path, starting with a side's power, and power that matters when it is gone.

Together these extend TGXngine from a single-machine engine to a networked one while preserving its modular and data-driven design philosophy.
