<br/>
<div align="center">
  <img alt="ForbocAI logo" src="https://forboc.ai/logo.png" height="50" align="center">

  <br/>

# ForbocAI SDK (UE 5.7)

`Engine_Compat // UE5_Lumen`

**ᚠ ᛫ ᛟ ᛫ ᚱ ᛫ ᛒ ᛫ ᛟ ᛫ ᚲ**

Autonomous AI for Unreal Engine 5.7.

[![Documentation](https://img.shields.io/badge/docs-docs.forboc.ai-blue)](https://docs.forboc.ai)
</div>

> *T̵h̶e̵ ̷f̵u̷t̶u̸r̸e̵ ̶i̷s̷ ̶n̷o̵w̷.̶ ̴T̷h̶e̸ ̸v̴o̵i̷d̸ ̷e̴x̵p̸a̷n̷d̵s̸.*

---

## Overview

`Córe_Módules // UE5_Plugin`

The **ForbocAI SDK for Unreal Engine 5.7** drives hyper-realistic NPC behavior through a neuro-symbolic AI architecture, written in strict **Functional C++11**.

### Modules

| Module | Purpose |
|--------|---------|
| **AgentModule** | Agent creation (factories), state management, AI processing |
| **BridgeModule** | Neuro-symbolic action validation against game rules |
| **MemoryModule** | Immutable memory store with embedding-based recall |
| **SoulModule** | Portable identity serialization (JSON import/export) |
| **CLIModule** | HTTP-based API operations for CLI commands |
| **Commandlet** | UE command-line interface for verification and admin |
| **functional_core.hpp** | Pure C++11 FP library: Maybe, Either, Currying, Lazy, Pipeline, Composition |

---

## Installation

`Instáll_Séquence // Fab_Dównload`

### Option 1: Fab (Recommended)

Get the plugin directly from **Fab** (formerly Unreal Engine Marketplace).

1.  Search for **ForbocAI** on Fab.
2.  Add to your library and install to the engine.
3.  Enable the plugin via `Edit > Plugins` in your project.

### Option 2: Manual

1.  Download the latest release.
2.  Copy the `ForbocAI_SDK` folder to your project's `Plugins/` directory.
3.  Regenerate project files and build.

---

## Quick Start

`Fáctory_Init // Agent_Créate`

**C++ — Using Factories and Free Functions (Strict FP):**

```cpp
#include "AgentModule.h"
#include "MemoryModule.h"

// 1. Create an agent via factory function (no constructors, no classes)
FAgentConfig Config;
Config.Persona = TEXT("Cyber-Merchant");
Config.ApiUrl = TEXT("https://api.forboc.ai");

const FAgent Merchant = AgentFactory::Create(Config);

// 2. Process input via free function
const FAgentResponse Response = AgentOps::Process(
    Merchant, TEXT("What wares do you have?"), {});

// 3. Functional update — returns a NEW agent (original unchanged)
const FAgentState NewState = TypeFactory::AgentState(TEXT("Suspicious"));
const FAgent UpdatedMerchant = AgentOps::WithState(Merchant, NewState);

// 4. Memory — immutable store, returns new store on add
const FMemoryStore Store = MemoryOps::CreateStore();
const FMemoryStore Updated = MemoryOps::Store(
    Store, TEXT("Customer asked about wares"), TEXT("interaction"), 0.8f);
```

**Pipeline chaining (from `functional_core.hpp`):**

```cpp
#include "Core/functional_core.hpp"

auto result = func::pipe(AgentFactory::Create(Config))
    | [](const FAgent& a) { return AgentOps::WithState(a, NewState); }
    | [](const FAgent& a) { return AgentOps::Export(a); };

FSoul soul = result.val;  // direct data access
```

---

## Command Line Interface

`CLI // Verification`

The SDK includes a built-in Commandlet for verification and administration.

### macOS

```bash
"/Users/Shared/Epic Games/UE_5.7/Engine/Binaries/Mac/UnrealEditor-Cmd" \
  "/Path/To/Your.uproject" \
  -run=ForbocAI_SDK -Command=doctor \
  -nosplash -nopause -unattended
```

### Windows

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "C:\Path\To\Your.uproject" `
  -run=ForbocAI_SDK -Command=doctor `
  -nosplash -nopause -unattended
```

### Linux

```bash
"/opt/UnrealEngine/Engine/Binaries/Linux/UnrealEditor-Cmd" \
  "/path/to/Your.uproject" \
  -run=ForbocAI_SDK -Command=doctor \
  -nosplash -nopause -unattended
```

> **Note:** The first run takes several minutes while the engine loads
> (dylib linking, DDC initialization, shader compilation). Subsequent runs
> are significantly faster (~30s engine startup).

**Commands:**
*   `doctor`: Check API connection status.
*   `agent_list`: List active agents.
*   `agent_create -Persona="..."`: Create a new agent.
*   `agent_process -Id="..." -Input="..."`: Interact with an agent.
*   `soul_export -Id="..."`: Export an agent's soul.

**Example `doctor` output:**
```
ForbocAI SDK CLI (UE5) - Command: doctor
Running Doctor check on https://api.forboc.ai...
API Status: ONLINE
Response: {"message":"Neuro-Symbolic Grid: ACTIVE","status":"online","version":"1.0.0"}
```

---

## Architecture & Concepts

`UE5_Arch // Fúnctional_C++11`

ForbocAI enforces **strict Functional Programming** in C++11. The entire SDK contains **no classes** (except UE-required `UCLASS` for the Commandlet). All code follows these rules:

| Rule | Pattern |
|------|---------|
| **Data** | `struct` only — public, immutable where possible, no member functions |
| **Construction** | Factory functions in namespaces (`AgentFactory::Create`, `TypeFactory::Soul`) |
| **Operations** | Free functions in namespaces (`AgentOps::Process`, `MemoryOps::Recall`) |
| **Updates** | Copy-on-write — always return a new value (`AgentOps::WithState`) |
| **Error handling** | `Maybe<T>` / `Either<E,T>` monads with free functions (`func::fmap`, `func::mbind`) |
| **Chaining** | `func::pipe(x) \| f \| g` or `func::compose(f, g)` |
| **Deferred work** | `func::lazy(thunk)` with `func::eval(lz)` |

**Key references:**
- **[Functional C++11 Guide](./C++11-FP-GUIDE.md)** — Patterns, rules, and examples
- **[`functional_core.hpp`](./Plugins/ForbocAI_SDK/Source/ForbocAI_SDK/Public/Core/functional_core.hpp)** — Production FP library
- **[`style-guide.md`](./style-guide.md)** — Aesthetic protocols

---

## License

`Légal_Státus // Ríghts`

All rights reserved. © 2026 ForbocAI. See [LICENSE](./LICENSE) for full details.
<!-- T̸h̴e̶ ̶v̶o̶i̶d̴ ̷c̸o̶n̷s̶u̶m̸e̸s̶ ̸a̶l̷l̵. -->
