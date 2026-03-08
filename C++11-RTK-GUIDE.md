I did some big updates to /Users/seandinwiddie/GitHub/sdk (see git history) and i need to bring /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-5.7.3 up to that level and then /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-4.23.1.

The main focus right now is documentation. (see /Users/seandinwiddie/GitHub/Forboc.AI/classified)

the first thing i want to do is make a rtk c++ shim /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-5.7.3/Plugins/ForbocAI_SDK/Source/ForbocAI_SDK/Public/Core/rtk.hpp. similar to the fp shim /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-5.7.3/Plugins/ForbocAI_SDK/Source/ForbocAI_SDK/Public/Core/functional_core.hpp

before any work is started for this, i want to well define what needs to be done.

what i want you to do is also glance at /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ts/rtk.md and then minic that in /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ue/rtk.md but whereas /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ts/rtk.md is for js/ts, /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ue/rtk.md should be for c++.

but the first thing i want you to do is populate /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ue/rtk-todo.md with what needs to be done for /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-5.7.3/Plugins/ForbocAI_SDK/Source/ForbocAI_SDK/Public/Core/rtk.hpp

so that is two files i want you to populate:

/Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ue/rtk-todo.md - a checklist of task items in order to populate /Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-5.7.3/Plugins/ForbocAI_SDK/Source/ForbocAI_SDK/Public/Core/rtk.hpp

and then /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ue/rtk.md - which should mirror /Users/seandinwiddie/GitHub/Forboc.AI/classified/docs/planning/sdk-devs/ts/rtk.md but for c++11 and unreal engine.