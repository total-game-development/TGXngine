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

---

## Version 0.4 — Multiplayer Networking

Version 0.4 promotes networking from long-term scope to the primary development target. The layer is built from the transport up: `Source/src/Net/` on the client, and the separate TGXngineServer repository on the other side of the socket. It also puts the shell on the network: every player's console is a computer the others can reach, and a player's buildings are processes running on it.

The reference implementation is the War of Salvation server, which runs the original RTS over deterministic lockstep. Its architecture sets the shape of this work.

### Deterministic Lockstep Simulation

Every player action becomes a command stamped with an execution tick and broadcast to all clients, which apply it at that tick. Simulation state is never transmitted — only input.

Delivered:

* Command stamping and a server tick clock.
* A command queue applied in arrival order at its stamped tick.
* Client pacing that runs a fixed buffer behind the last acknowledged server tick, with a cap on how many ticks one frame may simulate. Normal play never reaches it; a client replaying a match it joined part-way through does, and the cap is what keeps that replay from being one frame that never returns.
* Guarantees that no stamped command can arrive in the past.
* Desynchronization detection through periodic state digests. A client reports two: the commands it applied, held against the server's own fold, and its world, held against what the other clients report for the same tick.
* One stream of chance for the whole match, on `WorldState` where every module can reach it. A generator held static inside a header is a separate stream per module, seeded from the machine. Range reduction is written out rather than taken from `std::uniform_int_distribution`, whose mapping from engine output onto a range is unspecified and differs between standard libraries.

The consequence worth stating plainly is that a late reply is indistinguishable from a player with high latency. Nothing arriving over a socket may touch simulation state directly; it becomes a command or it does not happen.

### Client/Server Architecture

Delivered:

* A dedicated server process driving the tick.
* Room and match lifecycle management.
* Reconnection and late-join handling. A seat outlives the socket sitting in it: a client that drops holds its place, the clock stops rather than the match playing on without it, and the token it was issued brings it back. There is no world to send, so what a returning client is handed instead is the seed it started from and every command stamped since, which it replays. An observer joining a match already running takes the same path. A match nobody comes back to ends when the server's grace period runs out.
* Optional SSL transport, behind `-DTGX_ENABLE_TLS=ON` on both sides. Off by default: TLS needs OpenSSL or mbedTLS present, and a client that only ever speaks `ws://` should not fail to configure over a library it never calls. A `wss://` address in a build without it is refused with a message rather than failing silently.

Outstanding, and carried into 0.5:

* **The server holds no world.** `TickOnlySimulation` folds a real digest over real commands, which catches a client that applied a different command set, but there is nothing behind it to decide an outcome or to hold a world against. Hosting the engine's modules headlessly means running them without a `Window`, which they all currently require.

### Multiplayer Lobby

Delivered. The lobby lists rooms with their occupancy and map, and a room offers seats a player can move between, sides a player can take, a map the room can change, and readiness per seat. A match starts when every occupied seat has said yes, so a room of six can start a match between two. A side another player in the room holds is refused, and changing the map clears everybody's readiness, because a room that has agreed on a different map has not agreed to start on it. Watching is a first-class way in rather than a flag with no way to set it.

### What a Player Asks For

Only what a player asked for travels. Everything else on the event queue a match works out for itself, and every client works out the same thing on the same tick; sending those too would have each client raise its own copy and every client apply all of them.

Delivered: orders, carrying what was under the cursor rather than only where the click landed; placement; and paying for a unit. The last of these makes a team's treasury shared state rather than a number on one machine's HUD — both clients take the same amount off the same purse on the same tick — which is what lets the digest cover it.

Outstanding, and carried into 0.5: production queues are not shared state. The engine has one sidebar, belonging to the local player, so there is no per-team production model to keep in step. The timer on a button is local, and only the item it eventually places travels. A per-team production model belongs with the headless host.

### Networked AI

An AI commander need not run on the machine it plays from. Because decisions enter as ordinary commands, a remote AI cannot desynchronize the simulation regardless of how long it takes to answer or how non-deterministic it is. `modules/AI/` should be reachable over the same command path as a human player.

Not started, and carried into 0.5. A networked match currently erases the level's `ai` block outright, because a local commander would command the remote player's units as well as their owner does, from every client at once. Reaching the match over the command path instead depends on the headless host above.

### Remote Filesystems

Every player's console is a computer on the match's network, named for the side it plays. `hosts` lists the others, and `connect <side>` opens a session on one: `ls`, `tree`, `cd`, `mkdir`, `mk`, `del`, `rn` and `edit` then work on that player's filesystem rather than the local one, and the owner's console says who connected and when they left.

Delivered:

* Console traffic travels beside the command path, not on it. The server relays a `shell` message to the player seated on the side it names and never reads the body; nothing is stamped, nothing is folded, and nothing reaches the simulation, so no amount of it can desynchronize a match.
* Requests carry the working directory they were typed in, and the owner answers against that without moving their own. A request that goes unanswered times out, and one the server will not deliver is reported with the reason.
* Editing a remote file fetches it, opens it in the local editor, and writes it back on save.
* Limits that protect the socket a match is also travelling on: files up to 32 KB, and 20 messages a second per player. An observer has no computer and reaches none.
* The portal no longer pauses a networked match. A client that stopped polling while its console was open fell behind the room, and could not answer the other players' consoles.
* Cheats are refused in a networked match.

Access control, and running programs on another player's computer, are 0.6.

### Processes

A player's buildings are processes. Each one placed from the sidebar is given a PID when it goes up, in the order it was built, and is gone when it is destroyed. `ps` lists them with their state and what each supplies to or draws from the grid, alongside the programs `spawn` started. `kill` stops either, and `start` puts a stopped building back on the grid.

Delivered:

* Stopping a building takes it off its side's power grid, and starting it puts it back. In a networked match each is a stamped command like any order, applied on the same tick by every client, and a building's running state is folded into the world digest.
* Stopping a program interrupts it. Programs started by `spawn` ran on the task pool without a step budget, so a runaway one could not be ended; closing the shell also waited on it forever.
* Only a player's own buildings are listed, and only from their own console.

Power has no consequence beyond the grid readout yet; that arrives with 0.6, where cutting it is the point of a hack. Turrets are not processes until their behaviour depends on power.

### Prerequisite

Deterministic lockstep requires a deterministic update order. The `gameItems` and `world.items` consolidation is done: both are ordered by one total comparator and the two containers are no longer paired by index.

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

Version 0.4 set out to establish:

* Deterministic lockstep simulation across remote clients. **Done.**
* Client/server match hosting, including reconnection, late-join and optional TLS. **Done.**
* Networked lobby infrastructure: seats, sides, maps and readiness. **Done.**
* A single deterministic item update sequence underpinning all of the above. **Done.**
* Every player's console reachable as a computer on the match's network. **Done.**
* Buildings and programs as processes a player can list and stop. **Done.**
* AI commanders reachable over the command path. **Carried into 0.5**, with the headless host it depends on.

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
