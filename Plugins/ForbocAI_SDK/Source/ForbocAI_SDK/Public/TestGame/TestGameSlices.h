#pragma once
// Test-game slice definitions — mirrors TS test-game feature slices
// 13 slices across 5 domains: entities, mechanics, store, terminal, autoplay

#include "CoreMinimal.h"
#include "Core/rtk.hpp"
#include "TestGame/TestGameTypes.h"

namespace TestGame {

// =========================================================================
// Domain 1: Entities
// =========================================================================

// --- NPC Entity Adapter ---

inline rtk::EntityAdapterOps<FGameNPC> &GetNPCAdapter() {
  static rtk::EntityAdapterOps<FGameNPC> Adapter =
      rtk::createEntityAdapter<FGameNPC>(
          [](const FGameNPC &N) { return N.Id; });
  return Adapter;
}

struct FNPCsSliceState {
  rtk::EntityState<FGameNPC> Entities;
  FNPCsSliceState() : Entities(GetNPCAdapter().getInitialState()) {}
  bool operator==(const FNPCsSliceState &O) const {
    return Entities.ids == O.Entities.ids;
  }
};

namespace NPCsActions {

inline rtk::ActionCreator<FGameNPC> UpsertNPCActionCreator() {
  static auto C = rtk::createAction<FGameNPC>(TEXT("testgame/npcs/upsertNPC"));
  return C;
}

struct FMoveNPCPayload {
  FString Id;
  FPosition Position;
};

inline rtk::ActionCreator<FMoveNPCPayload> MoveNPCActionCreator() {
  static auto C =
      rtk::createAction<FMoveNPCPayload>(TEXT("testgame/npcs/moveNPC"));
  return C;
}

struct FPatchNPCPayload {
  FString Id;
  int32 Suspicion;
  bool bHasSuspicion;
  FPatchNPCPayload() : Suspicion(0), bHasSuspicion(false) {}
};

inline rtk::ActionCreator<FPatchNPCPayload> PatchNPCActionCreator() {
  static auto C =
      rtk::createAction<FPatchNPCPayload>(TEXT("testgame/npcs/patchNPC"));
  return C;
}

struct FApplyVerdictPayload {
  FString Id;
  FString ActionType;
  FPosition TargetHex;
  int32 SuspicionDelta;
  FApplyVerdictPayload() : SuspicionDelta(0) {}
};

inline rtk::ActionCreator<FApplyVerdictPayload>
ApplyNpcVerdictActionCreator() {
  static auto C = rtk::createAction<FApplyVerdictPayload>(
      TEXT("testgame/npcs/applyNpcVerdict"));
  return C;
}

inline rtk::AnyAction UpsertNPC(const FGameNPC &N) {
  return UpsertNPCActionCreator()(N);
}
inline rtk::AnyAction MoveNPC(const FMoveNPCPayload &P) {
  return MoveNPCActionCreator()(P);
}
inline rtk::AnyAction PatchNPC(const FPatchNPCPayload &P) {
  return PatchNPCActionCreator()(P);
}
inline rtk::AnyAction ApplyNpcVerdict(const FApplyVerdictPayload &P) {
  return ApplyNpcVerdictActionCreator()(P);
}

} // namespace NPCsActions

inline rtk::Slice<FNPCsSliceState> CreateNPCsSlice() {
  return rtk::SliceBuilder<FNPCsSliceState>(TEXT("testgame/npcs"),
                                            FNPCsSliceState())
      .addExtraCase(
          NPCsActions::UpsertNPCActionCreator(),
          [](const FNPCsSliceState &S,
             const rtk::Action<FGameNPC> &A) -> FNPCsSliceState {
            FNPCsSliceState Next = S;
            Next.Entities =
                GetNPCAdapter().upsertOne(Next.Entities, A.PayloadValue);
            return Next;
          })
      .addExtraCase(
          NPCsActions::MoveNPCActionCreator(),
          [](const FNPCsSliceState &S,
             const rtk::Action<NPCsActions::FMoveNPCPayload> &A)
              -> FNPCsSliceState {
            FNPCsSliceState Next = S;
            Next.Entities = GetNPCAdapter().updateOne(
                Next.Entities, A.PayloadValue.Id,
                [&A](const FGameNPC &Existing) {
                  FGameNPC Updated = Existing;
                  Updated.Position = A.PayloadValue.Position;
                  return Updated;
                });
            return Next;
          })
      .addExtraCase(
          NPCsActions::PatchNPCActionCreator(),
          [](const FNPCsSliceState &S,
             const rtk::Action<NPCsActions::FPatchNPCPayload> &A)
              -> FNPCsSliceState {
            FNPCsSliceState Next = S;
            Next.Entities = GetNPCAdapter().updateOne(
                Next.Entities, A.PayloadValue.Id,
                [&A](const FGameNPC &Existing) {
                  FGameNPC Updated = Existing;
                  if (A.PayloadValue.bHasSuspicion) {
                    Updated.Suspicion = A.PayloadValue.Suspicion;
                  }
                  return Updated;
                });
            return Next;
          })
      .addExtraCase(
          NPCsActions::ApplyNpcVerdictActionCreator(),
          [](const FNPCsSliceState &S,
             const rtk::Action<NPCsActions::FApplyVerdictPayload> &A)
              -> FNPCsSliceState {
            FNPCsSliceState Next = S;
            const auto &P = A.PayloadValue;
            Next.Entities = GetNPCAdapter().updateOne(
                Next.Entities, P.Id, [&P](const FGameNPC &Existing) {
                  FGameNPC Updated = Existing;
                  Updated.Suspicion += P.SuspicionDelta;
                  if (P.ActionType == TEXT("MOVE")) {
                    Updated.Position = P.TargetHex;
                  }
                  return Updated;
                });
            return Next;
          })
      .build();
}

// --- Player Slice ---

inline rtk::ActionCreator<FPosition> SetPositionActionCreator() {
  static auto C =
      rtk::createAction<FPosition>(TEXT("testgame/player/setPosition"));
  return C;
}

inline rtk::ActionCreator<bool> SetHiddenActionCreator() {
  static auto C = rtk::createAction<bool>(TEXT("testgame/player/setHidden"));
  return C;
}

namespace PlayerActions {
inline rtk::AnyAction SetPosition(const FPosition &P) {
  return SetPositionActionCreator()(P);
}
inline rtk::AnyAction SetHidden(bool H) {
  return SetHiddenActionCreator()(H);
}
} // namespace PlayerActions

inline rtk::Slice<FPlayerState> CreatePlayerSlice() {
  return rtk::SliceBuilder<FPlayerState>(TEXT("testgame/player"),
                                         FPlayerState())
      .addExtraCase(SetPositionActionCreator(),
                    [](const FPlayerState &S,
                       const rtk::Action<FPosition> &A) -> FPlayerState {
                      FPlayerState Next = S;
                      Next.Position = A.PayloadValue;
                      return Next;
                    })
      .addExtraCase(SetHiddenActionCreator(),
                    [](const FPlayerState &S,
                       const rtk::Action<bool> &A) -> FPlayerState {
                      FPlayerState Next = S;
                      Next.bHidden = A.PayloadValue;
                      return Next;
                    })
      .build();
}

// =========================================================================
// Domain 2: Mechanics
// =========================================================================

// --- Grid Slice ---

struct FSetGridSizePayload {
  int32 Width;
  int32 Height;
};

namespace GridActions {

inline rtk::ActionCreator<FSetGridSizePayload> SetGridSizeActionCreator() {
  static auto C =
      rtk::createAction<FSetGridSizePayload>(TEXT("testgame/grid/setGridSize"));
  return C;
}

inline rtk::ActionCreator<TArray<FPosition>> SetBlockedActionCreator() {
  static auto C = rtk::createAction<TArray<FPosition>>(
      TEXT("testgame/grid/setBlocked"));
  return C;
}

inline rtk::AnyAction SetGridSize(const FSetGridSizePayload &P) {
  return SetGridSizeActionCreator()(P);
}
inline rtk::AnyAction SetBlocked(const TArray<FPosition> &B) {
  return SetBlockedActionCreator()(B);
}

} // namespace GridActions

inline rtk::Slice<FGridState> CreateGridSlice() {
  return rtk::SliceBuilder<FGridState>(TEXT("testgame/grid"), FGridState())
      .addExtraCase(
          GridActions::SetGridSizeActionCreator(),
          [](const FGridState &S,
             const rtk::Action<FSetGridSizePayload> &A) -> FGridState {
            FGridState Next = S;
            Next.Width = A.PayloadValue.Width;
            Next.Height = A.PayloadValue.Height;
            return Next;
          })
      .addExtraCase(
          GridActions::SetBlockedActionCreator(),
          [](const FGridState &S,
             const rtk::Action<TArray<FPosition>> &A) -> FGridState {
            FGridState Next = S;
            Next.Blocked = A.PayloadValue;
            return Next;
          })
      .build();
}

// --- Stealth Slice ---

namespace StealthActions {

inline rtk::ActionCreator<bool> SetDoorOpenActionCreator() {
  static auto C =
      rtk::createAction<bool>(TEXT("testgame/stealth/setDoorOpen"));
  return C;
}

inline rtk::ActionCreator<int32> BumpAlertActionCreator() {
  static auto C =
      rtk::createAction<int32>(TEXT("testgame/stealth/bumpAlert"));
  return C;
}

inline rtk::AnyAction SetDoorOpen(bool V) {
  return SetDoorOpenActionCreator()(V);
}
inline rtk::AnyAction BumpAlert(int32 Delta) {
  return BumpAlertActionCreator()(Delta);
}

} // namespace StealthActions

inline rtk::Slice<FStealthState> CreateStealthSlice() {
  return rtk::SliceBuilder<FStealthState>(TEXT("testgame/stealth"),
                                          FStealthState())
      .addExtraCase(StealthActions::SetDoorOpenActionCreator(),
                    [](const FStealthState &S,
                       const rtk::Action<bool> &A) -> FStealthState {
                      FStealthState Next = S;
                      Next.bDoorOpen = A.PayloadValue;
                      return Next;
                    })
      .addExtraCase(StealthActions::BumpAlertActionCreator(),
                    [](const FStealthState &S,
                       const rtk::Action<int32> &A) -> FStealthState {
                      FStealthState Next = S;
                      int32 Raw = Next.AlertLevel + A.PayloadValue;
                      Next.AlertLevel =
                          FMath::Clamp(Raw, 0, 100);
                      return Next;
                    })
      .build();
}

// --- Social Slice ---

namespace SocialActions {

inline rtk::ActionCreator<FString> SetDialogueActionCreator() {
  static auto C =
      rtk::createAction<FString>(TEXT("testgame/social/setDialogue"));
  return C;
}

inline rtk::ActionCreator<FTradeOffer> SetTradeOfferActionCreator() {
  static auto C =
      rtk::createAction<FTradeOffer>(TEXT("testgame/social/setTradeOffer"));
  return C;
}

inline rtk::EmptyActionCreator ClearSocialStateActionCreator() {
  static auto C =
      rtk::createAction(TEXT("testgame/social/clearSocialState"));
  return C;
}

inline rtk::AnyAction SetDialogue(const FString &D) {
  return SetDialogueActionCreator()(D);
}
inline rtk::AnyAction SetTradeOffer(const FTradeOffer &T) {
  return SetTradeOfferActionCreator()(T);
}
inline rtk::AnyAction ClearSocialState() {
  return ClearSocialStateActionCreator()();
}

} // namespace SocialActions

inline rtk::Slice<FSocialState> CreateSocialSlice() {
  return rtk::SliceBuilder<FSocialState>(TEXT("testgame/social"),
                                         FSocialState())
      .addExtraCase(SocialActions::SetDialogueActionCreator(),
                    [](const FSocialState &S,
                       const rtk::Action<FString> &A) -> FSocialState {
                      FSocialState Next = S;
                      Next.ActiveDialogue = A.PayloadValue;
                      return Next;
                    })
      .addExtraCase(SocialActions::SetTradeOfferActionCreator(),
                    [](const FSocialState &S,
                       const rtk::Action<FTradeOffer> &A) -> FSocialState {
                      FSocialState Next = S;
                      Next.ActiveTrade = A.PayloadValue;
                      Next.bHasActiveTrade = true;
                      return Next;
                    })
      .addExtraCase(SocialActions::ClearSocialStateActionCreator(),
                    [](const FSocialState &S,
                       const rtk::Action<rtk::FEmptyPayload> &)
                        -> FSocialState { return FSocialState(); })
      .build();
}

// --- Bridge Slice (game-specific, not SDK BridgeSlice) ---

namespace GameBridgeActions {

inline rtk::ActionCreator<FBridgeRulesState> SetBridgeRulesActionCreator() {
  static auto C = rtk::createAction<FBridgeRulesState>(
      TEXT("testgame/bridge/setBridgeRules"));
  return C;
}

inline rtk::ActionCreator<FString> LoadBridgePresetActionCreator() {
  static auto C =
      rtk::createAction<FString>(TEXT("testgame/bridge/loadBridgePreset"));
  return C;
}

inline rtk::AnyAction SetBridgeRules(const FBridgeRulesState &R) {
  return SetBridgeRulesActionCreator()(R);
}
inline rtk::AnyAction LoadBridgePreset(const FString &P) {
  return LoadBridgePresetActionCreator()(P);
}

} // namespace GameBridgeActions

inline rtk::Slice<FBridgeRulesState> CreateGameBridgeSlice() {
  return rtk::SliceBuilder<FBridgeRulesState>(TEXT("testgame/bridge"),
                                              FBridgeRulesState())
      .addExtraCase(
          GameBridgeActions::SetBridgeRulesActionCreator(),
          [](const FBridgeRulesState &S,
             const rtk::Action<FBridgeRulesState> &A) -> FBridgeRulesState {
            return A.PayloadValue;
          })
      .addExtraCase(
          GameBridgeActions::LoadBridgePresetActionCreator(),
          [](const FBridgeRulesState &S,
             const rtk::Action<FString> &A) -> FBridgeRulesState {
            FBridgeRulesState Next = S;
            Next.ActivePreset = A.PayloadValue;
            if (A.PayloadValue == TEXT("social")) {
              Next.MaxMoveDistance = 1;
            }
            if (A.PayloadValue == TEXT("default")) {
              Next.MaxMoveDistance = 2;
            }
            return Next;
          })
      .build();
}

// =========================================================================
// Domain 3: Store (game-side memory, inventory, soul tracking)
// =========================================================================

// --- Memory Slice (game-specific) ---

inline rtk::EntityAdapterOps<FMemoryRecord> &GetGameMemoryAdapter() {
  static rtk::EntityAdapterOps<FMemoryRecord> Adapter =
      rtk::createEntityAdapter<FMemoryRecord>(
          [](const FMemoryRecord &R) { return R.Id; });
  return Adapter;
}

struct FGameMemorySliceState {
  rtk::EntityState<FMemoryRecord> Entities;
  FGameMemorySliceState()
      : Entities(GetGameMemoryAdapter().getInitialState()) {}
  bool operator==(const FGameMemorySliceState &O) const {
    return Entities.ids == O.Entities.ids;
  }
};

namespace GameMemoryActions {

inline rtk::ActionCreator<FMemoryRecord> StoreMemoryActionCreator() {
  static auto C =
      rtk::createAction<FMemoryRecord>(TEXT("testgame/memory/storeMemory"));
  return C;
}

inline rtk::ActionCreator<FString> ClearMemoryForNpcActionCreator() {
  static auto C =
      rtk::createAction<FString>(TEXT("testgame/memory/clearMemoryForNpc"));
  return C;
}

inline rtk::AnyAction StoreMemory(const FMemoryRecord &R) {
  return StoreMemoryActionCreator()(R);
}
inline rtk::AnyAction ClearMemoryForNpc(const FString &NpcId) {
  return ClearMemoryForNpcActionCreator()(NpcId);
}

} // namespace GameMemoryActions

inline rtk::Slice<FGameMemorySliceState> CreateGameMemorySlice() {
  return rtk::SliceBuilder<FGameMemorySliceState>(TEXT("testgame/memory"),
                                                  FGameMemorySliceState())
      .addExtraCase(
          GameMemoryActions::StoreMemoryActionCreator(),
          [](const FGameMemorySliceState &S,
             const rtk::Action<FMemoryRecord> &A) -> FGameMemorySliceState {
            FGameMemorySliceState Next = S;
            Next.Entities =
                GetGameMemoryAdapter().addOne(Next.Entities, A.PayloadValue);
            return Next;
          })
      .addExtraCase(
          GameMemoryActions::ClearMemoryForNpcActionCreator(),
          [](const FGameMemorySliceState &S,
             const rtk::Action<FString> &A) -> FGameMemorySliceState {
            FGameMemorySliceState Next = S;
            TArray<FString> ToRemove;
            for (const FString &Id : Next.Entities.ids) {
              if (const FMemoryRecord *R = Next.Entities.entities.Find(Id)) {
                if (R->NpcId == A.PayloadValue) {
                  ToRemove.Add(Id);
                }
              }
            }
            Next.Entities =
                GetGameMemoryAdapter().removeMany(Next.Entities, ToRemove);
            return Next;
          })
      .build();
}

// --- Inventory Slice ---

struct FSetOwnerInventoryPayload {
  FString OwnerId;
  TArray<FString> Items;
};

namespace InventoryActions {

inline rtk::ActionCreator<FSetOwnerInventoryPayload>
SetOwnerInventoryActionCreator() {
  static auto C = rtk::createAction<FSetOwnerInventoryPayload>(
      TEXT("testgame/inventory/setOwnerInventory"));
  return C;
}

inline rtk::AnyAction SetOwnerInventory(const FSetOwnerInventoryPayload &P) {
  return SetOwnerInventoryActionCreator()(P);
}

} // namespace InventoryActions

inline rtk::Slice<FInventoryState> CreateInventorySlice() {
  return rtk::SliceBuilder<FInventoryState>(TEXT("testgame/inventory"),
                                            FInventoryState())
      .addExtraCase(
          InventoryActions::SetOwnerInventoryActionCreator(),
          [](const FInventoryState &S,
             const rtk::Action<FSetOwnerInventoryPayload> &A)
              -> FInventoryState {
            FInventoryState Next = S;
            Next.ByOwner.Add(A.PayloadValue.OwnerId, A.PayloadValue.Items);
            return Next;
          })
      .build();
}

// --- Soul Tracking Slice ---

struct FMarkSoulExportedPayload {
  FString NpcId;
  FString TxId;
};

namespace GameSoulActions {

inline rtk::ActionCreator<FMarkSoulExportedPayload>
MarkSoulExportedActionCreator() {
  static auto C = rtk::createAction<FMarkSoulExportedPayload>(
      TEXT("testgame/soul/markSoulExported"));
  return C;
}

inline rtk::ActionCreator<FString> MarkSoulImportedActionCreator() {
  static auto C =
      rtk::createAction<FString>(TEXT("testgame/soul/markSoulImported"));
  return C;
}

inline rtk::AnyAction MarkSoulExported(const FMarkSoulExportedPayload &P) {
  return MarkSoulExportedActionCreator()(P);
}
inline rtk::AnyAction MarkSoulImported(const FString &TxId) {
  return MarkSoulImportedActionCreator()(TxId);
}

} // namespace GameSoulActions

inline rtk::Slice<FSoulTrackingState> CreateGameSoulSlice() {
  return rtk::SliceBuilder<FSoulTrackingState>(TEXT("testgame/soul"),
                                               FSoulTrackingState())
      .addExtraCase(
          GameSoulActions::MarkSoulExportedActionCreator(),
          [](const FSoulTrackingState &S,
             const rtk::Action<FMarkSoulExportedPayload> &A)
              -> FSoulTrackingState {
            FSoulTrackingState Next = S;
            Next.ExportsByNpc.Add(A.PayloadValue.NpcId, A.PayloadValue.TxId);
            return Next;
          })
      .addExtraCase(GameSoulActions::MarkSoulImportedActionCreator(),
                    [](const FSoulTrackingState &S,
                       const rtk::Action<FString> &A) -> FSoulTrackingState {
                      FSoulTrackingState Next = S;
                      Next.ImportedSoulTxIds.Add(A.PayloadValue);
                      return Next;
                    })
      .build();
}

// =========================================================================
// Domain 4: Terminal
// =========================================================================

// --- UI Slice ---

namespace UIActions {

inline rtk::ActionCreator<EPlayMode> SetModeActionCreator() {
  static auto C =
      rtk::createAction<EPlayMode>(TEXT("testgame/ui/setMode"));
  return C;
}

inline rtk::ActionCreator<FString> AddMessageActionCreator() {
  static auto C =
      rtk::createAction<FString>(TEXT("testgame/ui/addMessage"));
  return C;
}

inline rtk::AnyAction SetMode(EPlayMode M) {
  return SetModeActionCreator()(M);
}
inline rtk::AnyAction AddMessage(const FString &Msg) {
  return AddMessageActionCreator()(Msg);
}

} // namespace UIActions

inline rtk::Slice<FUIState> CreateUISlice() {
  return rtk::SliceBuilder<FUIState>(TEXT("testgame/ui"), FUIState())
      .addExtraCase(UIActions::SetModeActionCreator(),
                    [](const FUIState &S,
                       const rtk::Action<EPlayMode> &A) -> FUIState {
                      FUIState Next = S;
                      Next.Mode = A.PayloadValue;
                      return Next;
                    })
      .addExtraCase(UIActions::AddMessageActionCreator(),
                    [](const FUIState &S,
                       const rtk::Action<FString> &A) -> FUIState {
                      FUIState Next = S;
                      Next.Messages.Add(A.PayloadValue);
                      return Next;
                    })
      .build();
}

// --- Transcript Slice ---

namespace TranscriptActions {

struct FRecordTranscriptPayload {
  FString ScenarioId;
  ECommandGroup CommandGroup;
  FString Command;
  TArray<FString> ExpectedRoutes;
  ETranscriptStatus Status;
  FString Output;
};

inline rtk::ActionCreator<FRecordTranscriptPayload>
RecordTranscriptActionCreator() {
  static auto C = rtk::createAction<FRecordTranscriptPayload>(
      TEXT("testgame/transcript/recordTranscript"));
  return C;
}

inline rtk::EmptyActionCreator ResetTranscriptActionCreator() {
  static auto C =
      rtk::createAction(TEXT("testgame/transcript/resetTranscript"));
  return C;
}

inline rtk::AnyAction RecordTranscript(const FRecordTranscriptPayload &P) {
  return RecordTranscriptActionCreator()(P);
}
inline rtk::AnyAction ResetTranscript() {
  return ResetTranscriptActionCreator()();
}

} // namespace TranscriptActions

inline rtk::Slice<FTranscriptState> CreateTranscriptSlice() {
  return rtk::SliceBuilder<FTranscriptState>(TEXT("testgame/transcript"),
                                             FTranscriptState())
      .addExtraCase(
          TranscriptActions::RecordTranscriptActionCreator(),
          [](const FTranscriptState &S,
             const rtk::Action<TranscriptActions::FRecordTranscriptPayload> &A)
              -> FTranscriptState {
            FTranscriptState Next = S;
            FTranscriptEntry E;
            E.Id = FString::Printf(TEXT("%lld-%d"),
                                   FDateTime::Now().GetTicks(),
                                   FMath::Rand());
            E.ScenarioId = A.PayloadValue.ScenarioId;
            E.CommandGroup = A.PayloadValue.CommandGroup;
            E.Command = A.PayloadValue.Command;
            E.ExpectedRoutes = A.PayloadValue.ExpectedRoutes;
            E.Status = A.PayloadValue.Status;
            E.Output = A.PayloadValue.Output;
            E.Timestamp = FDateTime::Now().ToIso8601();
            Next.Entries.Add(E);
            return Next;
          })
      .addExtraCase(
          TranscriptActions::ResetTranscriptActionCreator(),
          [](const FTranscriptState &S,
             const rtk::Action<rtk::FEmptyPayload> &) -> FTranscriptState {
            return FTranscriptState();
          })
      .build();
}

// =========================================================================
// Domain 5: Autoplay
// =========================================================================

// --- Harness Slice ---

namespace HarnessActions {

inline rtk::ActionCreator<ECommandGroup> MarkCoveredActionCreator() {
  static auto C =
      rtk::createAction<ECommandGroup>(TEXT("testgame/harness/markCovered"));
  return C;
}

inline rtk::EmptyActionCreator ResetCoverageActionCreator() {
  static auto C =
      rtk::createAction(TEXT("testgame/harness/resetCoverage"));
  return C;
}

inline rtk::AnyAction MarkCovered(ECommandGroup G) {
  return MarkCoveredActionCreator()(G);
}
inline rtk::AnyAction ResetCoverage() {
  return ResetCoverageActionCreator()();
}

} // namespace HarnessActions

inline rtk::Slice<FHarnessState> CreateHarnessSlice() {
  return rtk::SliceBuilder<FHarnessState>(TEXT("testgame/harness"),
                                          FHarnessState())
      .addExtraCase(HarnessActions::MarkCoveredActionCreator(),
                    [](const FHarnessState &S,
                       const rtk::Action<ECommandGroup> &A) -> FHarnessState {
                      FHarnessState Next = S;
                      Next.Covered.Add(A.PayloadValue, true);
                      return Next;
                    })
      .addExtraCase(
          HarnessActions::ResetCoverageActionCreator(),
          [](const FHarnessState &S,
             const rtk::Action<rtk::FEmptyPayload> &) -> FHarnessState {
            return FHarnessState();
          })
      .build();
}

// --- Scenario Slice (read-only static data) ---

struct FScenarioSliceState {
  TArray<FScenarioStep> Steps;
  bool operator==(const FScenarioSliceState &O) const {
    return Steps.Num() == O.Steps.Num();
  }
};

// Forward declaration — scenarios defined in TestGameScenarios.h
TArray<FScenarioStep> GetDefaultScenarioSteps();

inline rtk::Slice<FScenarioSliceState> CreateScenarioSlice() {
  FScenarioSliceState Initial;
  Initial.Steps = GetDefaultScenarioSteps();
  return rtk::SliceBuilder<FScenarioSliceState>(TEXT("testgame/scenario"),
                                                Initial)
      .build();
}

// =========================================================================
// Selector: missing coverage groups
// =========================================================================

inline TArray<ECommandGroup>
SelectMissingGroups(const TMap<ECommandGroup, bool> &Covered) {
  TArray<ECommandGroup> Missing;
  for (ECommandGroup G : RequiredGroups()) {
    if (!Covered.Contains(G) || !Covered[G]) {
      Missing.Add(G);
    }
  }
  return Missing;
}

} // namespace TestGame
