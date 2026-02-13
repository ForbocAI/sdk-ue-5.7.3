<br/>
<div align="center">
  <img alt="ForbocAI logo" src="https://forboc.ai/logo.png" height="50" align="center">

  <br/>

# ForbocAI SDK (UE 5.7)

`Engine_Compat // UE5_Lumen`

**ᚠ ᛫ ᛟ ᛫ ᚱ ᛫ ᛒ ᛫ ᛟ ᛫ ᚲ**

Autonomous AI for Unreal Engine 5.7.3.

[![Documentation](https://img.shields.io/badge/docs-docs.forboc.ai-blue)](https://docs.forboc.ai)
</div>

> *T̵h̶e̵ ̷f̵u̷t̶u̸r̸e̵ ̶i̷s̷ ̶n̷o̵w̷.̶ ̴T̷h̶e̸ ̸v̴o̵i̷d̸ ̷e̴x̵p̸a̷n̷d̵s̸.*

---

## Overview

`Córe_Módules // UE5_Plugin`

The **ForbocAI SDK for Unreal Engine 5.7** leverages modern engine features to drive hyper-realistic NPC behavior.

- **Native C++17** — Modern standards for high-performance inference.
- **Subsystem Integration** — Global access via `UGameInstanceSubsystem`.
- **Async Blueprints** — Non-blocking Latent Actions for smooth gameplay integration.
- **Mass Entity Ready** — Designed for compatibility with Mass Entity systems.

---

## Installation

`Instáll_Séquence // Plúgin_Load`

1.  **Download**: Get the latest release for UE 5.7.3.
2.  **Copy**: Place the `ForbocAI` folder into your project's `Plugins/` directory.
    *   Example: `MyProject/Plugins/ForbocAI`
3.  **Build**: Run your build pipeline (or open modules in standard editor flow).
4.  **Enable**: Launch the editor and ensure "ForbocAI" is active in `Edit > Plugins`.

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

## Architecture

`UE5_Arch // Sübsystém`

UE 5 versions utilize `UGameInstanceSubsystem` for persistent AI state management across levels and streaming boundaries.

---

## Legal

`Légal_Státus // Ríghts`

All rights reserved. © 2026 ForbocAI
<!-- T̸h̴e̶ ̶v̶o̶i̶d̴ ̷c̸o̶n̷s̶u̶m̸e̸s̶ ̸a̶l̷l̵. -->
