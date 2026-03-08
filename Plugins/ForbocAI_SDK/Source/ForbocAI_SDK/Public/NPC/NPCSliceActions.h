#pragma once

#include "NPC/NPCTypes.h"

namespace NPCSlice {

struct FSetNPCStatePayload {
  FString Id;
  FAgentState State;
};

struct FUpdateNPCStatePayload {
  FString Id;
  FAgentState Delta;
};

struct FAddToHistoryPayload {
  FString Id;
  FString Role;
  FString Content;
};

struct FSetHistoryPayload {
  FString Id;
  TArray<FNPCHistoryEntry> History;
};

struct FSetLastActionPayload {
  FString Id;
  FAgentAction Action;
  bool bHasAction;
};

struct FBlockActionPayload {
  FString Id;
  FString Reason;
};

namespace Actions {

inline const rtk::ActionCreator<FNPCInternalState> &SetNPCInfoActionCreator() {
  static const rtk::ActionCreator<FNPCInternalState> ActionCreator =
      rtk::createAction<FNPCInternalState>(TEXT("npc/setNPCInfo"));
  return ActionCreator;
}

inline const rtk::ActionCreator<FString> &SetActiveNPCActionCreator() {
  static const rtk::ActionCreator<FString> ActionCreator =
      rtk::createAction<FString>(TEXT("npc/setActiveNPC"));
  return ActionCreator;
}

inline const rtk::ActionCreator<FSetNPCStatePayload> &
SetNPCStateActionCreator() {
  static const rtk::ActionCreator<FSetNPCStatePayload> ActionCreator =
      rtk::createAction<FSetNPCStatePayload>(TEXT("npc/setNPCState"));
  return ActionCreator;
}

inline const rtk::ActionCreator<FUpdateNPCStatePayload> &
UpdateNPCStateActionCreator() {
  static const rtk::ActionCreator<FUpdateNPCStatePayload> ActionCreator =
      rtk::createAction<FUpdateNPCStatePayload>(TEXT("npc/updateNPCState"));
  return ActionCreator;
}

inline const rtk::ActionCreator<FAddToHistoryPayload> &
AddToHistoryActionCreator() {
  static const rtk::ActionCreator<FAddToHistoryPayload> ActionCreator =
      rtk::createAction<FAddToHistoryPayload>(TEXT("npc/addToHistory"));
  return ActionCreator;
}

inline const rtk::ActionCreator<FSetHistoryPayload> &SetHistoryActionCreator() {
  static const rtk::ActionCreator<FSetHistoryPayload> ActionCreator =
      rtk::createAction<FSetHistoryPayload>(TEXT("npc/setHistory"));
  return ActionCreator;
}

inline const rtk::ActionCreator<FSetLastActionPayload> &
SetLastActionActionCreator() {
  static const rtk::ActionCreator<FSetLastActionPayload> ActionCreator =
      rtk::createAction<FSetLastActionPayload>(TEXT("npc/setLastAction"));
  return ActionCreator;
}

inline const rtk::ActionCreator<FBlockActionPayload> &
BlockActionActionCreator() {
  static const rtk::ActionCreator<FBlockActionPayload> ActionCreator =
      rtk::createAction<FBlockActionPayload>(TEXT("npc/blockAction"));
  return ActionCreator;
}

inline const rtk::ActionCreator<FString> &ClearBlockActionCreator() {
  static const rtk::ActionCreator<FString> ActionCreator =
      rtk::createAction<FString>(TEXT("npc/clearBlock"));
  return ActionCreator;
}

inline const rtk::ActionCreator<FString> &RemoveNPCActionCreator() {
  static const rtk::ActionCreator<FString> ActionCreator =
      rtk::createAction<FString>(TEXT("npc/removeNPC"));
  return ActionCreator;
}

inline rtk::AnyAction SetNPCInfo(const FNPCInternalState &Info) {
  return SetNPCInfoActionCreator()(Info);
}

inline rtk::AnyAction SetActiveNPC(const FString &Id) {
  return SetActiveNPCActionCreator()(Id);
}

inline rtk::AnyAction SetNPCState(const FString &Id, const FAgentState &State) {
  return SetNPCStateActionCreator()(FSetNPCStatePayload{Id, State});
}

inline rtk::AnyAction UpdateNPCState(const FString &Id,
                                     const FAgentState &Delta) {
  return UpdateNPCStateActionCreator()(FUpdateNPCStatePayload{Id, Delta});
}

inline rtk::AnyAction AddToHistory(const FString &Id, const FString &Role,
                                   const FString &Content) {
  return AddToHistoryActionCreator()(FAddToHistoryPayload{Id, Role, Content});
}

inline rtk::AnyAction SetHistory(const FString &Id,
                                 const TArray<FNPCHistoryEntry> &History) {
  return SetHistoryActionCreator()(FSetHistoryPayload{Id, History});
}

inline rtk::AnyAction SetLastAction(const FString &Id,
                                    const FAgentAction &LastAction,
                                    bool bHasAction = true) {
  return SetLastActionActionCreator()(
      FSetLastActionPayload{Id, LastAction, bHasAction});
}

inline rtk::AnyAction BlockAction(const FString &Id, const FString &Reason) {
  return BlockActionActionCreator()(FBlockActionPayload{Id, Reason});
}

inline rtk::AnyAction ClearBlock(const FString &Id) {
  return ClearBlockActionCreator()(Id);
}

inline rtk::AnyAction RemoveNPC(const FString &Id) {
  return RemoveNPCActionCreator()(Id);
}

} // namespace Actions

} // namespace NPCSlice
