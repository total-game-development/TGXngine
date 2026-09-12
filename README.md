# TGXngine

Tactical Game Engine: A cross-platform C++ real-time strategy (RTS) engine built with a modular architecture for data-driven gameplay and custom modding support.

TGXngine decouples stable engine fundamentals from flexible gameplay rules. The core executable handles system-level operations like windows, graphics, and inputs, while the actual game elements are built as separate, external modules. By loading these module files dynamically via JSON configurations, the engine acts as a plug-and-play platform. This allows developers and modders to build, tweak, and distribute custom gameplay mechanics as standalone mods without needing to modify or recompile the core engine itself.

![Alternative Text Descriptions](tgx_coverpage.png)

The engine's roots trace back to War of Salvation, the original RTS in this ecosystem, which was initially built specifically for the web. TGXngine was subsequently engineered as a major architectural extension to empower and involve the community, providing a high-performance native substrate for advanced development. A live web demo version of the original War of Salvation experience is available to play at https://tgame.dev/wos-game/.

## Version 0.3

Version 0.3 is implemented and tagged. The headline addition is an in-engine shell -- a filesystem, a scripting language and a code editor running in-process -- reachable both from the intro menu and from a portal raised over a live match. The strategy AI also stops keeping its own books.

### Shell Module (modules/Shell/)

The language and filesystem that War of Salvation ran in the browser are ported to a dynamic module: a `Lexer`, a recursive-descent `Parser` producing an `Ast`, and a tree-walking `Interpreter` over an `Environment` of `Value`s. A `FileSystem` holds directories and files in memory, a `Terminal` supplies the command set and its remote computers, and an `Editor` provides in-shell editing with `Highlight` colouring keywords, numbers, booleans and comments as they are typed. The four Web Workers behind `spawn` collapse into a `TaskPool`, so the runtime that `worker.js` duplicated inline exists once.

Programs reach the engine only through `Host` -- print, read, write, exec, spawn and toggle -- with toggle forwarded to a handler the executable registers, since a module cannot see `Globals`. Everything lives in `namespace TGX::Shell`. The filesystem persists to `Resources/shell.json` as it changes, so work survives a restart.

### UI Module (modules/UI/)

A second module supplies the in-match interface layer. A `Portal` reads `portal.json` and builds `Screen`s from declared `Element`s -- text, buttons, icon buttons and text inputs -- resolving positions through `Layout` expressions such as `centre-300` and `height-20` so a layout survives any view size. Opening a window attaches a `Panel` bound to a `Page`: a framed, draggable, closable window carrying a title, wrapped body text, an optional image and an optional inbox. The module is bound through `modules.json` as `{"type":"ui", "name":"modules/UI"}`, and maps declare their portal in their own `ui` blocks.

The toggle keys, opening screen, backdrop and console page are all declared in `portal.json` rather than compiled in. The portal pauses the match it covers.

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

The engine codebase is divided into three distinct structural layers: the Core Application, the Static Library Core, and external Dynamic Modules designed for modding.

### 1. The Core Application (The Executable Platform)

The executable handles OS-level window creation, rendering pipelines, user input routing, and multiplayer socket synchronization. It serves as the base application layer that reads data definitions and initializes game scenes.

Source implementation directories:

* src/Audio/
* src/Background/
* src/Grid/
* src/io/
* src/Scene/
* src/UI/
* src/WayPoints/

### 2. The Static Library Core (Low-Level Systems Engine)

Compiled directly into the application space, this layer provides stateless algorithms and mathematical structures used across the engine.

Implementation components:

* include/common/
* include/library/Collision/
* include/library/DataStructures/
* include/library/GameStructures/
* include/library/Heuristic/
* include/library/Node/
* include/library/PathFinding/
* include/library/Physics/
* include/library/Traversal/

### 3. The Dynamic Library Layer (Custom Modules and Modding)

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

### 4. Verification Frameworks

* tests/test_common/
* tests/test_library/
* tests/test_shell/

## Core Dependency Frameworks

The system depends on verified external vendor utilities embedded recursively inside the workflow:

* Windowing, Context & Graphics: SFML (Simple and Fast Multimedia Library)
* Real-time Networking Layers: IXWebSocket (Present but unused for future network implementations)
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
python3 build.py [-h] [-g] [-b] [-p] [-r] [-d] [-R] [-o] [-t] [-e] [-S] [generator]
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
```

## Quality Guidelines

We adhere strictly to formatting checks using clang-format. You can configure your local workstation parameters by executing:

```bash
python -m pip install clang-format
```
