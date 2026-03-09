#include "RuntimeCommandlet.h"
#include "CLI/CLIModule.h"
#include "Core/functional_core.hpp"
#include "Misc/Parse.h"
#include "RuntimeConfig.h"
#include <type_traits>

namespace {

FString NormalizeCommand(const FString &Command) {
  if (Command == TEXT("agent_list")) {
    return TEXT("npc_list");
  }
  if (Command == TEXT("agent_create")) {
    return TEXT("npc_create");
  }
  if (Command == TEXT("agent_process")) {
    return TEXT("npc_process");
  }
  return Command;
}

TArray<FString> BuildCommandArgs(const FString &Command, const FString &Params) {
  TArray<FString> Args;

  if (Command == TEXT("npc_create")) {
    FString Persona;
    FParse::Value(*Params, TEXT("Persona="), Persona);
    if (Persona.IsEmpty()) {
      Persona = TEXT("Default UE Persona");
    }
    Args.Add(Persona);
    return Args;
  }

  if (Command == TEXT("npc_process")) {
    FString Id;
    FString Input;
    FParse::Value(*Params, TEXT("Id="), Id);
    FParse::Value(*Params, TEXT("Input="), Input);
    if (!Id.IsEmpty()) {
      Args.Add(Id);
    }
    if (!Input.IsEmpty()) {
      Args.Add(Input);
    }
    return Args;
  }

  if (Command == TEXT("soul_export")) {
    FString Id;
    FParse::Value(*Params, TEXT("Id="), Id);
    if (!Id.IsEmpty()) {
      Args.Add(Id);
    }
    return Args;
  }

  if (Command == TEXT("config_set")) {
    FString Key;
    FString Value;
    FParse::Value(*Params, TEXT("Key="), Key);
    FParse::Value(*Params, TEXT("Value="), Value);
    if (!Key.IsEmpty()) {
      Args.Add(Key);
    }
    if (!Value.IsEmpty()) {
      Args.Add(Value);
    }
    return Args;
  }

  if (Command == TEXT("config_get")) {
    FString Key;
    FParse::Value(*Params, TEXT("Key="), Key);
    if (!Key.IsEmpty()) {
      Args.Add(Key);
    }
  }

  return Args;
}

} // namespace

UForbocAI_SDKCommandlet::UForbocAI_SDKCommandlet() {
  IsClient = false;
  IsEditor = false;
  IsServer = false;
  LogToConsole = true;
}

int32 UForbocAI_SDKCommandlet::Main(const FString &Params) {
  SDKConfig::InitializeConfig();

  FString Command;
  FParse::Value(*Params, TEXT("Command="), Command);
  Command = NormalizeCommand(Command);

  FString ApiUrl;
  FParse::Value(*Params, TEXT("ApiUrl="), ApiUrl);

  FString ApiKey;
  FParse::Value(*Params, TEXT("ApiKey="), ApiKey);

  if (!ApiUrl.IsEmpty() || !ApiKey.IsEmpty()) {
    SDKConfig::SetApiConfig(ApiUrl.IsEmpty() ? SDKConfig::GetApiUrl() : ApiUrl,
                            ApiKey.IsEmpty() ? SDKConfig::GetApiKey() : ApiKey);
  }

  UE_LOG(LogTemp, Display, TEXT("ForbocAI SDK CLI (UE5) - Command: %s"),
         *Command);

  const TArray<FString> Args = BuildCommandArgs(Command, Params);
  createCommandPipeline(Command, Args).execute();
  return 0;
}

UForbocAI_SDKCommandlet::CommandResult
UForbocAI_SDKCommandlet::executeCommand(const FString &Command,
                                        const TArray<FString> &Args) {
  return CLIOps::DispatchCommand(Command, Args);
}

UForbocAI_SDKCommandlet::CommandExecution
UForbocAI_SDKCommandlet::createCommandPipeline(const FString &Command,
                                               const TArray<FString> &Args) {
  return UForbocAI_SDKCommandlet::CommandExecution::create(
      [this, Command, Args](std::function<void()> Resolve,
                            std::function<void(std::string)> Reject) {
        const auto RejectMessage = [&Reject](const auto &Error) {
          using ErrorType = std::decay_t<decltype(Error)>;
          if constexpr (std::is_same_v<ErrorType, FString>) {
            Reject(TCHAR_TO_UTF8(*Error));
          } else {
            Reject(Error);
          }
        };

        const auto Validation = commandValidationPipeline().run(Command);
        if (Validation.isLeft) {
          RejectMessage(Validation.left);
          return;
        }

        const CommandResult Result = executeCommand(Command, Args);
        if (Result.isSuccessful()) {
          Resolve();
          return;
        }

        const FString ResultMessage =
            Result.message.empty()
                ? FString()
                : FString(UTF8_TO_TCHAR(Result.message.c_str()));
        RejectMessage(ResultMessage.IsEmpty()
                          ? FString::Printf(TEXT("Command failed: %s"), *Command)
                          : ResultMessage);
      });
}

CLITypes::ValidationPipeline<FString, FString>
UForbocAI_SDKCommandlet::commandValidationPipeline() {
  return CLITypes::ValidationPipeline<FString, FString>()
      .add([](const FString &Command) -> CLITypes::Either<FString, FString> {
        if (Command.IsEmpty()) {
          return CLITypes::make_left(FString(TEXT("Command cannot be empty")),
                                     FString());
        }
        return CLITypes::make_right(FString(), Command);
      })
      .add([](const FString &Command) -> CLITypes::Either<FString, FString> {
        static const TArray<FString> ValidCommands = {
            TEXT("doctor"),     TEXT("system_status"), TEXT("npc_list"),
            TEXT("npc_create"), TEXT("npc_process"),   TEXT("soul_export"),
            TEXT("config_set"), TEXT("config_get")};

        if (!ValidCommands.Contains(Command)) {
          return CLITypes::make_left(
              FString::Printf(TEXT("Invalid command: %s"), *Command),
              FString());
        }
        return CLITypes::make_right(FString(), Command);
      });
}
