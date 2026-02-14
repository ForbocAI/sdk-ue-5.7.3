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

The **ForbocAI SDK for Unreal Engine 5.7** leverages modern engine features to drive hyper-realistic NPC behavior.

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
2.  Copy the `ForbocAI` folder to your project's `Plugins/` directory.

---

## Quick Start

`Blúeprint_Async // Líve_Línk`

**Using the Subsystem:**
1.  Get the **ForbocAI Subsystem** from the Game Instance.
2.  Call `CreateAgent` to spawn a virtual mind.
3.  Use the **Async Action** node `Process Input` to handle dialogue without freezing the game thread.

```cpp
// C++ Example
UForbocAISubsystem* AI = GetGameInstance()->GetSubsystem<UForbocAISubsystem>();
auto Agent = AI->CreateAgent(TEXT("Cyber-Merchant"));
```

---

## Command Line Interface

`CLI // Verification`

The SDK includes a built-in Commandlet for verification and administration.

To run the CLI, use `UnrealEditor-Cmd` (paths may vary):

```bash
# macOS Example
"/Users/Shared/Epic Games/UE_5.7/Engine/Binaries/Mac/UnrealEditor-Cmd" \
  "/Path/To/Your.uproject" \
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

`UE5_Arch // Fúnctional_C++`

ForbocAI emphasizes a **Functional Programming** approach even within C++. We avoid traditional OOP heavy inheritance in favor of:

- **Immutable Data** (`struct` vs `class`)
- **Factory Functions** (vs Constructors)
- **Higher-Order Functions** (std::function, lambdas)

For a deep dive into writing Functional C++11 with this SDK, please read our **[Functional C++ Guide](./C++11-FP-GUIDE.md)** and the core library at [`functional_core.hpp`](./Plugins/ForbocAI_SDK/Source/ForbocAI_SDK/Public/Core/functional_core.hpp).

---

## Legal

`Légal_Státus // Ríghts`

All rights reserved. © 2026 ForbocAI
<!-- T̸h̴e̶ ̶v̶o̶i̶d̴ ̷c̸o̶n̷s̶u̶m̸e̸s̶ ̸a̶l̷l̵. -->
