#pragma once

#include "CoreMinimal.h"
#include "RuntimeConfig.h"
#include "RuntimeStore.h"
#include "Types.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RuntimeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNPCActionReceived, FAgentAction,
                                            Action);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMessageReceived, FString,
                                            Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTTSRequested, FString,
                                            Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTypingStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTypingEnd);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSoulExportComplete, FString,
                                            TxId);

/**
 * Forboc AI SDK Subsystem
 *
 * High-level Blueprint-friendly interface to the underlying RTK store and
 * thunks. Provides a bridge between Unreal's object-oriented visual scripting
 * and the functional Redux-style core.
 */
UCLASS(BlueprintType, Blueprintable)
class FORBOCAI_SDK_API UForbocAISubsystem : public UGameInstanceSubsystem {
  GENERATED_BODY()

public:
  virtual void Initialize(FSubsystemCollectionBase &Collection) override;
  virtual void Deinitialize() override;

  /**
   * Initializes the SDK with the provided configuration.
   */
  UFUNCTION(BlueprintCallable, Category = "Forboc AI|SDK")
  void Init(FString ApiKey, FString ApiUrl = TEXT("https://api.forboc.ai"));

  /**
   * Triggers the recursive protocol loop for an NPC.
   * This is an asynchronous operation.
   */
  UFUNCTION(BlueprintCallable, Category = "Forboc AI|NPC")
  void ProcessNPC(FString NpcId, FString Input = TEXT(""));

  /**
   * Exports an NPC's Soul to Arweave.
   */
  UFUNCTION(BlueprintCallable, Category = "Forboc AI|Soul")
  void ExportSoul(FString AgentId);

  /**
   * Gets the current state of an NPC.
   */
  UFUNCTION(BlueprintPure, Category = "Forboc AI|NPC")
  FAgentState GetNPCState(FString NpcId) const;

  /**
   * Gets the active NPC id, if any.
   */
  UFUNCTION(BlueprintPure, Category = "Forboc AI|NPC")
  FString GetActiveNPCId() const;

  /**
   * Gets the active NPC internal state, if any.
   */
  UFUNCTION(BlueprintPure, Category = "Forboc AI|NPC")
  bool GetActiveNPC(FNPCInternalState &OutNPC) const;

  /**
   * Gets the last recalled memories for the active NPC turn.
   * Wrapper over the memory slice selector for Blueprint consumers.
   */
  UFUNCTION(BlueprintPure, Category = "Forboc AI|Memory")
  TArray<FMemoryItem> GetLastRecalledMemories() const;

  /**
   * Gets the last bridge validation result, if present.
   */
  UFUNCTION(BlueprintPure, Category = "Forboc AI|Bridge")
  bool GetLastBridgeValidation(FValidationResult &OutResult) const;

  /**
   * Gets the last imported Soul, if present.
   */
  UFUNCTION(BlueprintPure, Category = "Forboc AI|Soul")
  bool GetLastImportedSoul(FSoul &OutSoul) const;

  /** Delegate triggered when a new action is received from the NPC. */
  UPROPERTY(BlueprintAssignable, Category = "Forboc AI|Events")
  FOnNPCActionReceived OnNPCActionReceived;

  /** Delegate triggered when finalized dialogue is produced for an NPC turn. */
  UPROPERTY(BlueprintAssignable, Category = "Forboc AI|Events")
  FOnMessageReceived OnMessageReceived;

  /** Delegate triggered when finalized dialogue is ready for TTS consumers. */
  UPROPERTY(BlueprintAssignable, Category = "Forboc AI|Events")
  FOnTTSRequested OnTTSRequested;

  /** Delegate triggered when an NPC turn begins asynchronous processing. */
  UPROPERTY(BlueprintAssignable, Category = "Forboc AI|Events")
  FOnTypingStart OnTypingStart;

  /** Delegate triggered when an NPC turn finishes asynchronous processing. */
  UPROPERTY(BlueprintAssignable, Category = "Forboc AI|Events")
  FOnTypingEnd OnTypingEnd;

  /** Delegate triggered when soul export is complete. */
  UPROPERTY(BlueprintAssignable, Category = "Forboc AI|Events")
  FOnSoulExportComplete OnSoulExportComplete;

private:
  /** The underlying functional Redux store. */
  TSharedPtr<rtk::EnhancedStore<FStoreState>> Store;

  /** Internal callback for the RTK listener middleware. */
  void HandleAction(const rtk::AnyAction &Action);
};
