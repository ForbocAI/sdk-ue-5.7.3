#include "ForbocAI_SDK_Commandlet.h"
#include "CLIModule.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

UForbocAI_SDKCommandlet::UForbocAI_SDKCommandlet() {
  IsClient = false;
  IsEditor = false;
  IsServer = false;
  LogToConsole = true;
}

int32 UForbocAI_SDKCommandlet::Main(const FString &Params) {
  // Parse Arguments
  // Format: -Command=<Cmd> -ApiUrl=<Url> ...

  FString Command;
  FParse::Value(*Params, TEXT("Command="), Command);

  FString ApiUrl = TEXT("https://api.forboc.ai");
  FParse::Value(*Params, TEXT("ApiUrl="), ApiUrl);

  UE_LOG(LogTemp, Display, TEXT("ForbocAI SDK CLI (UE5) - Command: %s"),
         *Command);

  // Command Routing
  if (Command == TEXT("doctor")) {
    CLIOps::Doctor(ApiUrl);
  } else if (Command == TEXT("agent_list")) {
    CLIOps::ListAgents(ApiUrl);
  } else if (Command == TEXT("agent_create")) {
    FString Persona;
    FParse::Value(*Params, TEXT("Persona="), Persona);
    if (Persona.IsEmpty())
      Persona = TEXT("Default UE Persona");

    CLIOps::CreateAgent(ApiUrl, Persona);
  } else if (Command == TEXT("agent_process")) {
    FString Id, Input;
    FParse::Value(*Params, TEXT("Id="), Id);
    FParse::Value(*Params, TEXT("Input="), Input);

    if (!Id.IsEmpty() && !Input.IsEmpty()) {
      CLIOps::ProcessAgent(ApiUrl, Id, Input);
    } else {
      UE_LOG(LogTemp, Error, TEXT("Missing Id or Input for agent_process"));
    }
  } else if (Command == TEXT("soul_export")) {
    FString Id;
    FParse::Value(*Params, TEXT("Id="), Id);

    if (!Id.IsEmpty()) {
      CLIOps::ExportSoul(ApiUrl, Id);
    }
  } else {
    UE_LOG(
        LogTemp, Warning,
        TEXT(
            "Unknown Command or No Command Specified. usage: -Command=doctor"));
  }

  // HTTP requests are now handled synchronously via SendRequestAndWait
  // in CLIModule, which pumps the HTTP tick loop until completion.

  return 0;
}
