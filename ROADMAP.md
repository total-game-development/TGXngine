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

Carried into 0.5, and delivered there: a world on the server, AI commanders over the command path, and production as shared state.

### Version 0.5 — Arena Mode

* Headless hosting (`src/Host.cpp`, `src/Replay.cpp`) — `TGXngine --host URL --room N --token T` joins a room as its host, runs every tick the players run and reports its checksums, with no window at any point: `WorldState::SetHeadless` stops `Window` creating one, and a headless run takes no console, so it cannot overwrite the players' `Resources/shell.json`. `TGXngine --replay FILE` runs a match the server recorded the same way, and holds its world against every checksum the clients reported. A networked frame is one method, `Game::AdvanceNetworked`, and a networked tick another, `Game::RunTick`, so a live client, the host and a replay cannot drift apart in the order they run a tick.
* The host's world as the reference — a server started with `--engine PATH` runs an engine for every match, and holds each player's world fold against the host's rather than against another player's, checking whichever of the two reports second. The players are still held against one another as well, so a room whose host is late or gone is no less checked than before. This closes the world-fold half carried from 0.4.
* A server-decided outcome — the host reads what is still standing, reports who won or that nobody did, and the server ends the match with it. Everybody watching is told.
* Rules auditing (`src/Rules.h`) — the digest catches a world that differs from another machine's, not one that is illegal on every machine at once. The host audits the occupancy grid every 60 ticks, `--audit` setting the interval, for units come to rest on ground another holds, stacks nothing accounts for, and bodies or tactical bookings held by uids nothing alive carries, and closes with a tally. A player's client is untouched: this is the authoritative world checking itself.
* Networked AI — an AI commander no longer changes the world. What it decides goes into `WorldState::aiCommands` as a command and travels the command path: applied at once in single player, stamped by the server in a networked match, so it reaches every client on the same tick like a player's order. Only the match's host runs the commander, for the sides nobody sits on. Money a purchase has committed stays counted against the purse until the purchase lands, so a commander reading a world behind its own orders cannot decide the same thing twice.
* Arena rooms — `join_arena` on an empty room opens it as an arena, seats the host's AI on every side and asks nobody whether they are ready. A match is dealt once two people are watching, the next after an eight-second break, and the room is an ordinary room again when the last viewer leaves. Anybody arriving later watches part-way through, by the replay path 0.4 built; nobody can take a seat. `A` over a room in the lobby opens one, and the arena has its own place on the main menu.
* Per-team production — what a side is making is `WorldState::productionOrders`, not a float on the button of the player who clicked. Every client advances every side's orders; `PlayerProduce` pays for an order and refuses one the side cannot afford or is already making; a finished unit deploys itself from the building that made it, and a finished building waits, ready, until `PlayerPlace` says where it goes, so a placement with no finished order behind it builds nothing. The orders are folded into the world digest beside the treasuries. This closes the last of the 0.4 carry.

Beyond the plan, 0.5 carries the first work aimed at running the engine inside Unreal: `build.py -R -b --export` writes a release into the Unreal project's external content; `--skirmish`, `--map` and `--team` start a match from the command line instead of the menu; `--fps` and `--frametimes` separate what the engine spends from what presentation does; and a borderless windowed mode composites through the DWM rather than taking the display outright, which is what a match drawn beside an editor needs.

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
* The server accepts a hack against a side only from that side's own connection, and only while an authorised session into its computer is open. A tampered client could still ignore hacks against itself; closing that means having 0.5's headless host decide them instead.

### Radar Cracking

Chambers et al., *Mitigating Information Exposure to Cheaters in Real-Time Strategy Games* (NOSSDAV 2005), hash the positions of units hidden in the fog of war so that an opponent cannot learn them. Version 0.6 turns that mechanism into an objective: every player's computer publishes its positions hashed, and breaking the hash is something the other side sets out to do.

This is a game mechanic, not anti-cheat. Every lockstep client simulates every unit, so the positions are already in memory on every machine; the lock is drawn for somebody playing through the console to pick, and holds against nobody editing their own client. Hiding positions from a client for real needs a server-authoritative world, which is listed under Future Scope.

Planned functionality:

* A radar on every player's computer. `/sys/radar` names the map and the cell size, carries a check hash of the computer's salt, and lists one salted hash for every cell its own units and buildings stand on. It is republished every few seconds, and the salt is drawn from a generator local to the console, never from the simulation's stream.
* `hash(...)` and `reveal(salt, x, y)` in the shell language. `hash` is FNV-1a over its arguments joined by colons, the same function the radar uses, so a player can write the cracker themselves: find the salt from the check, then hash every cell and match.
* `get <file>` copies a file from a computer you are connected to. A copy of the live radar is kept aside as it arrived, so `reveal` is checked against what the owner actually published rather than against a file the caller can edit.
* A proved cell lifts the fog around it on the caller's own view for a few seconds. It is presentation, like the fog, and never reaches the simulation.
* A hacker sees the board through fog it cannot see through: it borrows a side's perspective to draw with, but not its sight, so everything it sees it cracked.
* `rekey` on the owner's console draws a new salt, and the salt rotates on its own on a timer. A side whose grid is cut can do neither, which is what joins the two hacks: cut a grid, and the radar behind it stays cracked.
* A level's `radar` block sets the salt length, the cell size, how often the radar is republished and rotated, and how far and for how long a reveal shows. A coarse cell reveals an area rather than a unit's cell.

Supported by the design, and left for red-versus-blue rounds to decide:

* Handing what was cracked to a side. A cracked cell is shown only on the console that proved it; a `leak <side>` would send the proof on, and the check against the owner's published radar is what it would reuse.
* Keeping reveals as simulation state, stamped and folded, so that something other than the view could act on them.

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

### Server-Authoritative Visibility

Hide what a player cannot see from the player's own machine, rather than from the view drawn on it. Lockstep clients simulate every unit and so hold every position; a client that holds only what its side can see needs the host's world to be the authority and each client to be sent its share, which is a different networking model from 0.4's. Version 0.6's radar cracking would then guard real information, and the Chambers et al. scheme would be anti-cheat rather than an objective.

---

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

Version 0.5 established:

* The engine's modules hosted headlessly, so the server drives a world rather than a clock.
* A server-decided outcome, and a client's world checked against the host's rather than against another client's.
* The authoritative world audited against the game's own rules, not only against the other clients.
* AI commanders reachable over the command path.
* Arena rooms, where commanders play one another and people watch.
* Production as shared state rather than local interface state.

Version 0.6 aims to establish:

* Access control on every player's computer.
* Programs run on another player's computer, with their output returned.
* Hacks that reach the match through the command path, starting with a side's power, and power that matters when it is gone.
* A radar on every computer that can be cracked for the positions behind it, and rekeyed to make the crack stale.

Together these extend TGXngine from a single-machine engine to a networked one while preserving its modular and data-driven design philosophy.
