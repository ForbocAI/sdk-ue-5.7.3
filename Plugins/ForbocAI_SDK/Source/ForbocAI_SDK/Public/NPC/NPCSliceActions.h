#pragma once

#include "NPC/NPCTypes.h"

namespace NPCSlice {

// --- Actions ---
struct SetNPCInfoAction : public Action {
  static const FString Type;
  FNPCInternalState Payload;
  SetNPCInfoAction(FNPCInternalState InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString SetNPCInfoAction::Type = TEXT("npc/setNPCInfo");

struct SetActiveNPCAction : public Action {
  static const FString Type;
  FString Payload;
  SetActiveNPCAction(FString InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString SetActiveNPCAction::Type = TEXT("npc/setActiveNPC");

struct SetNPCStateAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    FAgentState State;
  };
  FPayload Payload;
  SetNPCStateAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString SetNPCStateAction::Type = TEXT("npc/setNPCState");

struct UpdateNPCStateAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    FAgentState StateDelta;
  };
  FPayload Payload;
  UpdateNPCStateAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString UpdateNPCStateAction::Type = TEXT("npc/updateNPCState");

struct AddToHistoryAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    FString Entry;
  };
  FPayload Payload;
  AddToHistoryAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString AddToHistoryAction::Type = TEXT("npc/addToHistory");

struct SetHistoryAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    TArray<FString> History;
  };
  FPayload Payload;
  SetHistoryAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString SetHistoryAction::Type = TEXT("npc/setHistory");

struct SetLastActionAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    FAgentAction Action;
  };
  FPayload Payload;
  SetLastActionAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString SetLastActionAction::Type = TEXT("npc/setLastAction");

struct BlockActionAction : public Action {
  static const FString Type;
  struct FPayload {
    FString Id;
    FString Reason;
  };
  FPayload Payload;
  BlockActionAction(FPayload InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString BlockActionAction::Type = TEXT("npc/blockAction");

struct ClearBlockAction : public Action {
  static const FString Type;
  FString Payload;
  ClearBlockAction(FString InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString ClearBlockAction::Type = TEXT("npc/clearBlock");

struct RemoveNPCAction : public Action {
  static const FString Type;
  FString Payload;
  RemoveNPCAction(FString InPayload)
      : Action(Type), Payload(MoveTemp(InPayload)) {}
};
inline const FString RemoveNPCAction::Type = TEXT("npc/removeNPC");

namespace Actions {
inline SetNPCInfoAction SetNPCInfo(FNPCInternalState Info) {
  return SetNPCInfoAction(MoveTemp(Info));
}
inline SetActiveNPCAction SetActiveNPC(FString Id) {
  return SetActiveNPCAction(MoveTemp(Id));
}
inline SetNPCStateAction SetNPCState(FString Id, FAgentState State) {
  return SetNPCStateAction({MoveTemp(Id), MoveTemp(State)});
}
inline UpdateNPCStateAction UpdateNPCState(FString Id, FAgentState Delta) {
  return UpdateNPCStateAction({MoveTemp(Id), MoveTemp(Delta)});
}
inline AddToHistoryAction AddToHistory(FString Id, FString Entry) {
  return AddToHistoryAction({MoveTemp(Id), MoveTemp(Entry)});
}
inline SetHistoryAction SetHistory(FString Id, TArray<FString> History) {
  return SetHistoryAction({MoveTemp(Id), MoveTemp(History)});
}
inline SetLastActionAction SetLastAction(FString Id, FAgentAction LastAction) {
  return SetLastActionAction({MoveTemp(Id), MoveTemp(LastAction)});
}
inline BlockActionAction BlockAction(FString Id, FString Reason) {
  return BlockActionAction({MoveTemp(Id), MoveTemp(Reason)});
}
inline ClearBlockAction ClearBlock(FString Id) {
  return ClearBlockAction(MoveTemp(Id));
}
inline RemoveNPCAction RemoveNPC(FString Id) {
  return RemoveNPCAction(MoveTemp(Id));
}
} // namespace Actions

} // namespace NPCSlice
