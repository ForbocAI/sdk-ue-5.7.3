#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "Core/functional_core.hpp"
#include "ForbocAI_SDK_Commandlet.generated.h"

/**
 * ForbocAI SDK Commandlet.
 * Usage:
 *   ./UnrealEditorCmd -run=ForbocAI_SDK -Command=<CommandName> [Args...]
 *
 * Commands:
 *   doctor
 *   agent_list
 *   agent_create -Persona="..."
 *   agent_process -Id="..." -Input="..."
 *   soul_export -Id="..."
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

  // Command execution helpers
  CommandResult executeDoctor(const FString& apiUrl);
  CommandResult executeAgentList(const FString& apiUrl);
  CommandResult executeAgentCreate(const FString& apiUrl, const FString& persona);
  CommandResult executeAgentProcess(const FString& apiUrl, const FString& id, const FString& input);
  CommandResult executeSoulExport(const FString& apiUrl, const FString& id);

  // Command pipeline helpers
  CommandExecution createCommandPipeline(const FString& command, const FString& apiUrl, const FString& param1 = TEXT(""), const FString& param2 = TEXT(""));

  // Command validation helpers
  CLITypes::ValidationPipeline<FString, FString> commandValidationPipeline();
};