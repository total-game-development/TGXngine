# TGXngine

Tactical Game Engine: A cross-platform C++ real-time strategy (RTS) engine built with a modular architecture for data-driven gameplay and custom modding support.

TGXngine decouples stable engine fundamentals from flexible gameplay rules. The core executable handles system-level operations like windows, graphics, and inputs, while the actual game elements are built as separate, external modules. By loading these module files dynamically via JSON configurations, the engine acts as a plug-and-play platform. This allows developers and modders to build, tweak, and distribute custom gameplay mechanics as standalone mods without needing to modify or recompile the core engine itself.

![Alternative Text Descriptions](tgx_coverpage.png)

The engine's roots trace back to War of Salvation, the original RTS in this ecosystem, which was initially built specifically for the web. TGXngine was subsequently engineered as a major architectural extension to empower and involve the community, providing a high-performance native substrate for advanced development. A live web demo version of the original War of Salvation experience is available to play at https://tgame.dev/wos-game/.

## Version 0.5

Version 0.5 is implemented and tagged. The engine gets a world with nobody at a keyboard behind it: the server runs a headless copy of the game beside every match, holds the players to it, and decides how the match ends. With a world there, the AI can play from it -- an arena is a room where commanders play one another and people watch.

### Headless Hosting (src/Host.cpp, src/Replay.cpp)

`TGXngine --host URL --room N --token T` joins a room as its host. It takes the match the way an observer does, runs every tick the players run, reports its checksums for the server to hold theirs against, and tells the server who won once only one side has anything left standing. The server starts it; nobody runs it by hand. `TGXngine --replay FILE` runs a match the server recorded the same way, and exits 0 only if it arrives at every world the clients reported.

Neither opens a window. `WorldState::SetHeadless` is read by `Window` before it creates one, and the same flag keeps a headless run off the console, so a host cannot overwrite the `Resources/shell.json` the players on that machine are using. A networked frame is one method, `Game::AdvanceNetworked`, and a networked tick another, `Game::RunTick`, so the live client, the host and a replay cannot drift apart in the order they do things.

A server started with `--engine PATH` runs one of these per match. Every player's world fold is now held against the host's rather than against another player's -- whichever of the two reports second is the one checked, since the host replays its way in and can report either side of them. The players are still checked against one another as well, so a room whose host is late or gone is no less checked than before.

### Rules Auditing (src/Rules.h)

A digest catches a world that differs from another machine's. It cannot catch one that is illegal on every machine at once: infantry that comes to rest inside a vehicle folds to the same number everywhere and passes. `Rules::Check` reads the occupancy grid, which carries a body only while its unit stands still, and reports two vehicles in one cell, anything stopped inside a building or turret, a stack no body accounts for, and a body or tactical booking held by a uid nothing alive carries. The host audits every 60 ticks and closes with what it found over how many checks; `--audit` sets the interval. A player's client is untouched -- this is the authoritative world checking itself, where a match with nobody at a keyboard runs long enough for the drift to show.

### Networked AI

An AI commander no longer changes the world. What it decides -- a purchase, a build, a wave -- goes into `WorldState::aiCommands` as a command, and the game takes it from there: applied at once in single player, sent to be stamped in a networked match, so it reaches every client on the same tick like a player's order. Only the match's host runs the commander, and only for the sides nobody sits on; every other client sees those sides as players it cannot see. A command takes a few ticks to come back stamped, so money a purchase has committed stays counted against the purse until it lands and a build is in flight until the building is in the world, which is what stops a commander reading a world behind its own orders from deciding the same thing twice.

### Arena Rooms

`A` over a room in the lobby sends `join_arena`, and the arena has its own place on the main menu. An empty room opens as an arena: the host's AI takes every side and nobody is asked whether they are ready. A match is dealt once two people are watching, since a match dealt to one viewer is a match nobody shares, and the next follows an eight-second break, long enough to read who won. Anybody arriving later watches part-way through, by the same replay path a dropped player comes back on; nobody can take a seat. When the last viewer leaves the match ends and the room is an ordinary room again. A server with no `--engine` refuses to open one.

### Per-Team Production

What a side was making lived on one machine's sidebar: a float on the button of the player who clicked, run on that client's frame time. It is now `WorldState::productionOrders`, one order per side and item, advanced every tick by every client. `PlayerProduce` pays for an order and starts it, refusing one the side cannot afford or is already making; a finished unit deploys itself from the building that made it, and a finished building or turret waits, ready, until `PlayerPlace` says where it goes, so a placement with no finished order behind it builds nothing. The orders are folded into the world digest beside the treasuries. The sidebar keeps its buttons but no timers -- a button reads its side's order from the world.

### Running a Match Without the Menu

`--skirmish` starts a match instead of the intro, taking a map by name or by its place among the skirmish maps, with `--map` saying the same when the flag is left bare and `--team` picking the side to command. `--fps` and `--frametimes` report the frame rate and trace where a frame went, in production as well as debug. `--production` plays fullscreen whatever `settings.json` says, and a `borderless` key composites a desktop-sized window through the DWM instead of taking the display outright, which is what a match drawn beside another application needs. `build.py -R -b --export` writes a release into the Unreal project's external content.

## Version 0.4

Version 0.4 is implemented and tagged. The engine goes onto the network: matches between machines over deterministic lockstep, against the separate TGXngineServer, with a lobby to arrange them in. The shell follows it there -- every player's console is a computer the others can reach, and a player's buildings run on it as processes.

### Deterministic Lockstep (src/Net/)

Only input travels. Every order a player gives becomes a command, the server stamps it with the tick it is to run on, and every client applies it on that tick; the world itself is never sent. `Lockstep` holds each client a fixed buffer behind the last tick the server announced, so a stamped command cannot arrive late, and caps how many ticks one frame may run, which is what lets a client replay a whole match to catch up without freezing. Chance is one stream seeded by the server, held on `WorldState` where every module draws from it, with range reduction written out rather than left to the standard library.

Each client reports two digests on a fixed cadence: the commands it applied, which the server holds against its own fold, and its world, which is held against the other clients'. Either disagreeing is reported as a desync. Orders carry what was under the cursor as well as where the click landed, placement and paying for a unit travel as commands too, and a team's treasury is therefore the same number on every machine.

### Server and Lobby

`Session` is the one place that speaks the server's protocol, and a `Multiplayer` scene drives it before a match. The lobby lists rooms with their occupancy and map; a room offers seats, sides, a map and readiness per seat, and starts when every occupied seat is ready. A seat outlives its socket: a client that drops holds its place and the clock stops for it, and the token it was issued brings it back to replay the match from its seed. Observers join the same way, including part-way through. TLS is available behind `-DTGX_ENABLE_TLS=ON`, and `TGX_SERVER` points a client at a server on another machine.

### The Shell On the Network

A player's console is a computer named for their side. `hosts` lists the others, and `connect <side>` opens a session on one, after which the filesystem commands and `edit` work on that player's files; the owner's console says who connected. The traffic rides beside the command path rather than on it: the server relays it to the named player without reading it, and nothing in it is stamped, folded or seen by the simulation, so it cannot desynchronize a match. Messages are rate-limited and files capped at 32 KB, since they share a socket with the match. Cheats are refused in a networked match, and the portal no longer pauses one -- a client that stopped reading its socket fell behind the room.

### Processes

A player's buildings are processes, each given a PID as it goes up and gone when it is destroyed. `ps` lists them with what each supplies to or draws from the power grid, alongside programs started with `spawn`. `kill` and `start` take a building off the grid and put it back, as stamped commands in a networked match; `kill` also interrupts a program, which could not previously be stopped at all.

## Version 0.3

Version 0.3 is implemented and tagged. The headline addition is an in-engine shell -- a filesystem, a scripting language and a code editor running in-process -- reachable both from the intro menu and from a portal raised over a live match. The strategy AI also stops keeping its own books.

### Shell Module (modules/Shell/)

The language and filesystem that War of Salvation ran in the browser are ported to a dynamic module: a `Lexer`, a recursive-descent `Parser` producing an `Ast`, and a tree-walking `Interpreter` over an `Environment` of `Value`s. A `FileSystem` holds directories and files in memory, a `Terminal` supplies the command set and its remote computers, and an `Editor` provides in-shell editing with `Highlight` colouring keywords, numbers, booleans and comments as they are typed. The four Web Workers behind `spawn` collapse into a `TaskPool`, so the runtime that `worker.js` duplicated inline exists once.

Programs reach the engine only through `Host` -- print, read, write, exec, spawn and toggle -- with toggle forwarded to a handler the executable registers, since a module cannot see `Globals`. Everything lives in `namespace TGX::Shell`. The filesystem persists to `Resources/shell.json` as it changes, so work survives a restart.

### UI Module (modules/UI/)

A second module supplies the in-match interface layer. A `Portal` reads `portal.json` and builds `Screen`s from declared `Element`s -- text, buttons, icon buttons and text inputs -- resolving positions through `Layout` expressions such as `centre-300` and `height-20` so a layout survives any view size. Opening a window attaches a `Panel` bound to a `Page`: a framed, draggable, closable window carrying a title, wrapped body text, an optional image and an optional inbox. The module is bound through `modules.json` as `{"type":"ui", "name":"modules/UI"}`, and maps declare their portal in their own `ui` blocks.

The toggle keys, opening screen, backdrop and console page are all declared in `portal.json` rather than compiled in. The portal pauses the match it covers in single player. A networked match cannot stop for one player, so there the portal is an overlay and the clock carries on underneath it.

### The Shell In a Match

The two modules never call one another. The `Game` scene holds both handles and brokers between them: it asks the UI where the console window sits, hands the Shell that rectangle as a viewport, and forwards the keys the portal reports unhandled. `Shell::Draw` anchors to that viewport rather than to the window, so the intro menu's full-screen shell is unchanged by the arrangement.

### AI Economy

`BuilderAIState` previously spent a private figure handed to it at load, so it could build past an empty treasury and its income never appeared in the game's own books. It now reads and debits the team's `EconomyInstance`, taking payment when an order starts rather than when it completes, so the same funds cannot be committed twice. With a shared purse the commander gained limits -- `armyLimit` and `waveSize`, both declarable per opponent -- and a wave now forms only from units actually idle at base. Each tick it publishes an `AIDebugSnapshot` to `WorldState`, which an on-screen readout draws: balance, income, forces against the cap, waves sent, the order on the slab, and why nothing is being built when nothing is.

## Version 0.2

Version 0.2 is implemented and tagged. The headline additions are a modular strategy AI and a standalone skirmish mode, alongside the modules that extend play beyond the land domain.

### Modular Strategy AI (modules/AI/)

The opponent ships as a dynamic module like any other gameplay layer, so its behaviour can be replaced or extended without recompiling the core engine. It is built on a polymorphic state architecture: an `AIState` base with `BuilderAIState` driving base development and build-order execution through a `BuildPlanner`, and `PlexAIState` handling the strategic layer above it. Role demand is resolved dynamically against whatever modules are loaded, so an `AIProfile` only tracks roles the current module set can actually field. The module is bound through `modules.json` as `{"type":"ai", "name":"modules/AI"}`, and maps declare their AI teams in their own `ai` blocks.

### Skirmish Mode

A skirmish lobby scene configures a match before it starts: map selection, per-slot team and role assignment, spectator mode, and start or cancel. The lobby writes its result into `SkirmishSetup`, which the game scene reads when building the match. Five skirmish maps ship with the engine: plains, island, snow, desert and water.

### Supporting Modules

Version 0.2 also lands the Aircrafts, Ships, FogOfWar and Turrets modules, extending the engine to air and naval domains, battlefield visibility, and defensive structures.

## Engine Architecture and Codebase Structure

The engine codebase is divided into four distinct structural layers: the Core Application, the Shared Common Layer, the Static Library Core, and external Dynamic Modules designed for modding.

![TGXngine engine structure: the core executable loads the dynamic modules, both link Common and Library, and everything builds on the vendored dependencies](tgx_structure.png)

### 1. The Core Application (The Executable Platform)

The executable handles OS-level window creation, rendering pipelines, user input routing, and multiplayer socket synchronization. It serves as the base application layer that reads data definitions and initializes game scenes.

Source implementation directories:

* src/Audio/
* src/Background/
* src/Grid/
* src/io/
* src/Net/
* src/Scene/
* src/UI/
* src/WayPoints/

### 2. The Shared Common Layer (Common.dll)

Built as a shared library that the executable and every module load, so all of them see a single copy of the world. It holds the shared world state, the instance types, orders and navigation, platform services such as the window and image loading, and the module ABI (`module_interface.h`) that modules export their entry points through.

Implementation components:

* include/common/

### 3. The Static Library Core (Low-Level Systems Engine)

Compiled directly into the executable and into each module, this layer provides stateless algorithms and mathematical structures used across the engine.

Implementation components:

* include/library/Collision/
* include/library/DataStructures/
* include/library/GameStructures/
* include/library/Heuristic/
* include/library/Node/
* include/library/PathFinding/
* include/library/Physics/
* include/library/Traversal/

### 4. The Dynamic Library Layer (Custom Modules and Modding)

This layer encapsulates gameplay logic inside isolated dynamic libraries. This decoupling allows developers and community modders to write entirely new unit behaviors, faction mechanics, or game triggers as self-contained mods.

Isolated dynamic modules available in version 0.3:

* modules/AI/
* modules/Aircrafts/
* modules/Buildings/
* modules/Economy/
* modules/FogOfWar/
* modules/Infantry/
* modules/Interface/
* modules/Projectiles/
* modules/Resources/
* modules/Shell/
* modules/Ships/
* modules/Triggers/
* modules/Turrets/
* modules/UI/
* modules/Vehicles/

### 5. Verification Frameworks

* tests/test_common/
* tests/test_library/
* tests/test_net/
* tests/test_rules/
* tests/test_shell/

## Core Dependency Frameworks

The system depends on verified external vendor utilities embedded recursively inside the workflow:

* Windowing, Context & Graphics: SFML (Simple and Fast Multimedia Library)
* Real-time Networking Layers: IXWebSocket (the transport under `Source/src/Net/`; build with `-DTGX_ENABLE_TLS=ON` for `wss://`)
* High-Speed Serialization: nlohmann-json
* Optimal Associative Containers: ankerl::unordered_dense
* String Formatting Pipeline: {fmt}
* Pseudo-Random Number Distributions: effolkronium random

## Environment Set Up

### 1. Repository Procurement

Clone the framework recursively to fetch all dependency trees, then pull runtime binary components:

```bash
git clone https://github.com/total-game-development/TGXngine.git
cd TGXngine
git submodule update --init --recursive
git lfs pull
```

### 2. Platform Tooling Prerequisites

#### Windows

Note: TGXngine is a 64-bit architecture project. It is highly recommended to use PowerShell to execute these setup commands to ensure proper path resolution, architecture matching, and package permissions.

Visual Studio 2026 is required with the **Desktop Development with C++** workload installed. TGXngine targets the C++23 standard and requires a compiler toolchain with full C++23 support.

Install your toolchain configurations via winget and fetch the required 64-bit audio, font, and compression runtime prerequisites with vcpkg:

```bash
winget install cmake python3
vcpkg install zlib

# Manual ZLib Fallback Assembly (If required)
cd Vendor/zlib && mkdir build && cd build
cmake ..
cmake --build . --config Release
cmake --install .
cd ../../..
```

#### macOS

Ensure development toolchains are provisioned via Homebrew:

```bash
brew install cmake python zlib
xcode-select --install
```

#### Linux (Ubuntu / Debian derivatives)

Update packages and gather essential compilation environments:

```bash
sudo apt update
sudo apt install cmake gcc g++ make python3 zlib1g-dev
```

## Building & Running the Project

Compilation, testing, and debugging loops are automated using a centralized python automation wrapper (build.py).

```bash
# General CLI Command Template
python3 build.py [-h] [-g] [-b] [-p] [-r] [-d] [-R] [-o] [-F] [-t] [-e] [-S] [generator]
```

### Script Execution Parameters

* -h, --help: Show help message and exit.
* -g, --generate: Regenerate project files inside build/<generator>. Required on initialization.
* -b, --build: Build target matching current generation.
* -p, --publish: Copy build files to /War-of-Salvation.
* -r, --run: Run result binary.
* -d, --docs: Build documentation with Doxygen.
* -R, --release: Build target in Release mode.
* -o, --optimise: Build with -O3 optimisation.
* -F, --fast-debug: Optimise the Debug build while keeping its symbols. Stepping becomes jumpy and some locals are optimised away, so plain Debug is still the one to use when a breakpoint has to land exactly where it was put.
* -t, --test: Build and run unit tests.
* -e, --examine: Examine a crash using GDB/LLDB (Linux/macOS).
* -S, --sanitizers: Generate project with ASan/UBSan (Linux/macOS).

## Common Local Build Recipes

```bash
# Generate the project:
python3 build.py -g

# Build and run the project:
python3 build.py -b -r

# Run the binary:
python3 build.py -r

# Generate, Build and run the project in one:
python3 build.py -g -b -r

# Build and Release project:
python3 build.py -b -R

# Judge whether a change is fast enough without leaving the Debug build:
python3 build.py -g -F -b -r
```

Both -o and -F are settled at generation time, so they need -g; a build alone
will not pick them up. A Debug build runs an order of magnitude under a Release
one, which makes it a poor place to judge the speed of anything and painful in a
match paced by a clock. -F closes most of that gap: over the unit tests, naval
pathfinding drops from 25ms to 3ms and the shell interpreter from 11ms to 2ms.

## Quality Guidelines

We adhere strictly to formatting checks using clang-format. You can configure your local workstation parameters by executing:

```bash
python -m pip install clang-format
```
