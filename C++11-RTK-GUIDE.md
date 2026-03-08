I did some big updates to /Users/seandinwiddie/GitHub/sdk (see git history) and i need to bring /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-5.7.3 up to that level and then /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-4.23.1.

The main focus right now is documentation. (see /Users/seandinwiddie/GitHub/Forboc.AI/classified)

the first thing i want to do is make a rtk c++ shim /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-5.7.3/Plugins/ForbocAI_SDK/Source/ForbocAI_SDK/Public/Core/rtk.hpp. similar to the fp shim /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-5.7.3/Plugins/ForbocAI_SDK/Source/ForbocAI_SDK/Public/Core/functional_core.hpp

before any work is started for this, i want to well define what needs to be done.

what i want you to do is also glance at /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ts/rtk.md and then minic that in /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ue/rtk.md but whereas /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ts/rtk.md is for js/ts, /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ue/rtk.md should be for c++.

but the first thing i want you to do is populate /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ue/rtk-todo.md with what needs to be done for /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-5.7.3/Plugins/ForbocAI_SDK/Source/ForbocAI_SDK/Public/Core/rtk.hpp

so that is two files i want you to populate:

/Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ue/rtk-todo.md - a checklist of task items in order to populate /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-5.7.3/Plugins/ForbocAI_SDK/Source/ForbocAI_SDK/Public/Core/rtk.hpp

and then /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ue/rtk.md - which should mirror /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ts/rtk.md but for c++11 and unreal engine.

---

please update /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ue/rtk-todo.md that /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-5.7.3/Plugins/ForbocAI_SDK/Source/ForbocAI_SDK/Public/Core/rtk.hpp should `#include "Core/functional_core.hpp"` and employ/wield /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-5.7.3/Plugins/ForbocAI_SDK/Source/ForbocAI_SDK/Public/Core/functional_core.hpp for it's logic.

---

please add to /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ue/TODO.md what will need to be done once /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-5.7.3/Plugins/ForbocAI_SDK/Source/ForbocAI_SDK/Public/Core/rtk.hpp is populated in order to bring /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-5.7.3 upto date to /Users/seandinwiddie/GitHub/sdk excluding the browser package. and in /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-5.7.3/ we will not separate the packages. we just want what is essencally in the core and node package and maybe also what is in the test-game package i am not sure yet

---

please add emence amount of commentary to /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ue/TODO.md                  
  /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ue/rtk-todo.md so that it is not left up to interpitation as to the   
  nuance of what needs to be done so that even a junior or oop senior programmer can follow it and not mess up that accruate implementation.    
  think of yourself as a hybryd of https://github.com/markerikson, https://github.com/wadler, https://github.com/sebh  

do this in pyrimd style passes, with each pass getting more elaborative

think of it like a 10% pass each time

-

     Plan: Add Exhaustive Implementation Commentary to UE TODO Files

     Context

     The two UE SDK planning documents (TODO.md and rtk-todo.md) contain task checklists but lack the nuance and detail needed for a developer unfamiliar with the ForbocAI architecture to implement them correctly. The goal is to add immense commentary — written from the combined perspective of Mark Erikson (Redux Toolkit creator), Philip Wadler (FP type theory), and Sebastien Hillaire (UE rendering/systems expert) —
     so that even a junior developer or an OOP-trained senior can follow each item without misinterpreting what needs to be built.

     The commentary will be added in pyramid-style passes (10 passes at ~10% each), where each pass adds a deeper layer of elaboration on top of the previous.

     Files to Modify

     1. /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ue/TODO.md
     2. /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ue/rtk-todo.md

     Reference Files (read-only, for accuracy)

     - TS SDK slices: ~/GitHub/sdk/packages/core/src/{npcSlice,memorySlice,directiveSlice,cortexSlice,ghostSlice,bridgeSlice,soulSlice,apiSlice,listeners,errors,thunks,store}.ts
     - TS SDK node: ~/GitHub/sdk/packages/node/src/{nodeCortexSlice,nodeMemorySlice,nodeVectorSlice,nodeConfig,store}.ts
     - TS SDK types: ~/GitHub/sdk/packages/core/src/types/{protocol,cortex,memory,bridge,ghost,soul,system}.ts
     - C++ FP core: ~/GitHub/Forboc.AI/sdk-ue-5.7.3/Plugins/ForbocAI_SDK/Source/ForbocAI_SDK/Public/Core/functional_core.hpp

     Approach

     For each TODO item in both files, add layered commentary covering:

     Pass 1-2 (20%): Intent & Context

     - What this item does and why it exists
     - Which TS SDK file it maps to

     Pass 3-4 (40%): Data Contracts & Signatures

     - Exact struct shapes, field names, field types
     - Factory function signatures
     - Which func:: types to use (Maybe, Either, AsyncResult, etc.)

     Pass 5-6 (60%): Implementation Mechanics

     - Step-by-step pseudocode or algorithm description
     - Threading concerns (Game Thread vs. async)
     - UE-specific patterns (USTRUCT, FString, TArray, TMap)

     Pass 7-8 (80%): Edge Cases & Pitfalls

     - Common OOP mistakes to avoid (don't add member functions, don't use inheritance)
     - FString<->std::string conversion overhead warnings
     - Immutability enforcement patterns
     - What NOT to do (explicit anti-patterns)

     Pass 9-10 (100%): Cross-References & Verification

     - How each item connects to other items
     - How to test in isolation
     - Exact TS SDK line references for behavior parity checks

     Commentary Voice

     - Erikson: Practical Redux patterns, "here's why the abstraction exists", normalized state, action naming conventions
     - Wadler: Type-theoretic correctness, monadic chaining, parametric polymorphism, "if it compiles it should be correct"
     - Hillaire: UE engine realities, frame budget, Game Thread constraints, USTRUCT/UPROPERTY interop, memory layout

     Execution

     1. Rewrite rtk-todo.md with full commentary on all 10 phases (store foundation through testing)
     2. Rewrite TODO.md with full commentary on critical priority items, open items, Phase 4 audit, and all POST-RTK phases (A through I)
     3. Preserve existing checkbox state and document structure
     4. Preserve aesthetic protocol compliance headers

     Verification

     - Diff both files to confirm no checkbox items were lost
     - Grep for all TS SDK action names mentioned and verify they match actual TS source
     - Confirm all func:: type references exist in functional_core.hpp