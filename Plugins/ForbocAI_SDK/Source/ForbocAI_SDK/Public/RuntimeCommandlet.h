#pragma once

#include "CLI/CLIModule.h"
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "Core/functional_core.hpp"
#include "RuntimeCommandlet.generated.h"

namespace CLITypes {
using func::AsyncResult;
using func::Either;
using func::TestResult;
using func::ValidationPipeline;
using func::make_left;
using func::make_right;
} // namespace CLITypes

/**
 * ForbocAI SDK Commandlet.
 * Usage:
 *   ./UnrealEditorCmd -run=ForbocAI_SDK -Command=<CommandName> [Args...]
 *
 * Canonical commands:
 *   doctor
 *   npc_list
 *   npc_create -Persona="..."
 *   npc_process -Id="..." -Input="..."
 *   soul_export -Id="..."
 *   config_set -Key="..." -Value="..."
 *   config_get -Key="..."
 */
UCLASS()
class UForbocAI_SDKCommandlet : public UCommandlet {
  GENERATED_BODY()

public:
  UForbocAI_SDKCommandlet();

  //~ Begin UCommandlet Interface
  virtual int32 Main(const FString &Params) override;
  //~ End UCommandlet Interface

  // Functional Core Type Aliases for Commandlet operations
  using CommandResult = CLITypes::TestResult<void>;
  using CommandExecution = CLITypes::AsyncResult<void>;

  CommandResult executeCommand(const FString &Command,
                               const TArray<FString> &Args);

  // Command pipeline helpers
  CommandExecution createCommandPipeline(const FString &Command,
                                         const TArray<FString> &Args);

  // Command validation helpers
  CLITypes::ValidationPipeline<FString, FString> commandValidationPipeline();
};
