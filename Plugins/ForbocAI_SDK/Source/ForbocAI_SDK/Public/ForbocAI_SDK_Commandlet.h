#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
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
};
