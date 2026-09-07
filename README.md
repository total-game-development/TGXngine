# TGXngine

Tactical Game Engine: A cross-platform C++ real-time strategy (RTS) engine built with a modular architecture for data-driven gameplay and custom modding support.

TGXngine decouples stable engine fundamentals from flexible gameplay rules. The core executable handles system-level operations like windows, graphics, and inputs, while the actual game elements are built as separate, external modules. By loading these module files dynamically via JSON configurations, the engine acts as a plug-and-play platform. This allows developers and modders to build, tweak, and distribute custom gameplay mechanics as standalone mods without needing to modify or recompile the core engine itself.

![Alternative Text Descriptions](tgx_coverpage.png)

The engine's roots trace back to War of Salvation, the original RTS in this ecosystem, which was initially built specifically for the web. TGXngine was subsequently engineered as a major architectural extension to empower and involve the community, providing a high-performance native substrate for advanced development. A live web demo version of the original War of Salvation experience is available to play at https://tgame.dev/wos-game/.

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

Isolated dynamic modules available in version 0.2:

* modules/AI/
* modules/Aircrafts/
* modules/Buildings/
* modules/Economy/
* modules/FogOfWar/
* modules/Infantry/
* modules/Interface/
* modules/Projectiles/
* modules/Resources/
* modules/Ships/
* modules/Triggers/
* modules/Turrets/
* modules/Vehicles/

### 4. Verification Frameworks

* tests/test_common/
* tests/test_library/

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
