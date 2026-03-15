#include "RuntimeCommandlet.h"
#include "CLI/CLIModule.h"
#include "Core/functional_core.hpp"
#include "Misc/Parse.h"
#include "RuntimeConfig.h"
#include <type_traits>

namespace {

/**
 * Normalizes legacy agent-prefixed commands to the canonical npc names.
 * User Story: As CLI compatibility, I need old command aliases mapped forward
 * so legacy scripts keep working with the renamed NPC command surface.
 */
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
  if (Command == TEXT("agent_active")) {
    return TEXT("npc_active");
  }
  if (Command == TEXT("agent_state")) {
    return TEXT("npc_state");
  }
  if (Command == TEXT("agent_update")) {
    return TEXT("npc_update");
  }
  if (Command == TEXT("agent_import")) {
    return TEXT("npc_import");
  }
  if (Command == TEXT("agent_chat")) {
    return TEXT("npc_chat");
  }
  return Command;
}

/**
 * Extracts a named param from UE command-line params string.
 * Returns empty string if not found.
 * User Story: As commandlet parsing, I need named parameter extraction so raw
 * Unreal params can be translated into structured command arguments.
 */
FString ExtractParam(const FString &Params, const TCHAR *ParamName) {
  FString Value;
  FParse::Value(*Params, ParamName, Value);
  return Value;
}

/**
 * Appends a param value to the args array if non-empty.
 * User Story: As commandlet parsing, I need optional args appended only when
 * present so handlers receive a clean positional argument list.
 */
void AddIfPresent(TArray<FString> &Args, const FString &Value) {
  if (!Value.IsEmpty()) {
    Args.Add(Value);
  }
}

/**
 * Builds positional command arguments from raw Unreal commandlet params.
 * User Story: As command dispatch, I need raw commandlet params converted into
 * handler-friendly args so CLI routing can reuse the shared command handlers.
 */
TArray<FString> BuildCommandArgs(const FString &Command, const FString &Params) {
  TArray<FString> Args;

  /**
   * ---- NPC ----
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  if (Command == TEXT("npc_create")) {
    FString Persona = ExtractParam(Params, TEXT("Persona="));
    Args.Add(Persona.IsEmpty() ? TEXT("Default UE Persona") : Persona);
    return Args;
  }
  if (Command == TEXT("npc_process")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Id=")));
    AddIfPresent(Args, ExtractParam(Params, TEXT("Input=")));
    return Args;
  }
  if (Command == TEXT("npc_update")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Id=")));
    AddIfPresent(Args, ExtractParam(Params, TEXT("Mood=")));
    AddIfPresent(Args, ExtractParam(Params, TEXT("Inventory=")));
    return Args;
  }
  if (Command == TEXT("npc_import")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("TxId=")));
    return Args;
  }
  if (Command == TEXT("npc_chat")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Id=")));
    AddIfPresent(Args, ExtractParam(Params, TEXT("Message=")));
    return Args;
  }

  /**
   * ---- Memory (remote) ----
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  if (Command == TEXT("memory_list") || Command == TEXT("memory_clear") ||
      Command == TEXT("memory_export")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Id=")));
    return Args;
  }
  if (Command == TEXT("memory_recall")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Id=")));
    AddIfPresent(Args, ExtractParam(Params, TEXT("Query=")));
    return Args;
  }
  if (Command == TEXT("memory_store")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Id=")));
    AddIfPresent(Args, ExtractParam(Params, TEXT("Obs=")));
    return Args;
  }

  /**
   * ---- Cortex ----
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  if (Command == TEXT("cortex_init")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Model=")));
    return Args;
  }
  if (Command == TEXT("cortex_init_remote")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Model=")));
    AddIfPresent(Args, ExtractParam(Params, TEXT("Key=")));
    return Args;
  }
  if (Command == TEXT("cortex_complete")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Id=")));
    AddIfPresent(Args, ExtractParam(Params, TEXT("Prompt=")));
    return Args;
  }

  /**
   * ---- Ghost ----
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  if (Command == TEXT("ghost_run")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Suite=")));
    AddIfPresent(Args, ExtractParam(Params, TEXT("Duration=")));
    return Args;
  }
  if (Command == TEXT("ghost_status") || Command == TEXT("ghost_results") ||
      Command == TEXT("ghost_stop")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("SessionId=")));
    return Args;
  }
  if (Command == TEXT("ghost_history")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Limit=")));
    return Args;
  }

  /**
   * ---- Bridge ----
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  if (Command == TEXT("bridge_validate")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Action=")));
    return Args;
  }
  if (Command == TEXT("bridge_preset")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Name=")));
    return Args;
  }

  /**
   * ---- Rules ----
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  if (Command == TEXT("rules_register")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Json=")));
    return Args;
  }
  if (Command == TEXT("rules_delete")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Id=")));
    return Args;
  }

  /**
   * ---- Soul ----
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  if (Command == TEXT("soul_export")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Id=")));
    return Args;
  }
  if (Command == TEXT("soul_import") || Command == TEXT("soul_import_npc") ||
      Command == TEXT("soul_verify")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("TxId=")));
    return Args;
  }
  if (Command == TEXT("soul_list")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Limit=")));
    return Args;
  }

  /**
   * ---- Config ----
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  if (Command == TEXT("config_set")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Key=")));
    AddIfPresent(Args, ExtractParam(Params, TEXT("Value=")));
    return Args;
  }
  if (Command == TEXT("config_get")) {
    AddIfPresent(Args, ExtractParam(Params, TEXT("Key=")));
    return Args;
  }

  return Args;
}

} // namespace

UForbocAICommandlet::UForbocAICommandlet() {
  IsClient = false;
  IsEditor = false;
  IsServer = false;
  LogToConsole = true;
}

/**
 * Runs the commandlet entrypoint for the requested CLI command.
 * User Story: As Unreal CLI execution, I need one main entrypoint so command
 * parameters can be validated, normalized, and dispatched consistently.
 */
int32 UForbocAICommandlet::Main(const FString &Params) {
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

  UE_LOG(LogTemp, Display, TEXT("ForbocAI CLI (UE5) - Command: %s"),
         *Command);

  const TArray<FString> Args = BuildCommandArgs(Command, Params);
  createCommandPipeline(Command, Args).execute();
  return 0;
}

UForbocAICommandlet::CommandResult
UForbocAICommandlet::executeCommand(const FString &Command,
                                    const TArray<FString> &Args) {
  return CLIOps::DispatchCommand(Command, Args);
}

UForbocAICommandlet::CommandExecution
UForbocAICommandlet::createCommandPipeline(const FString &Command,
                                           const TArray<FString> &Args) {
  return UForbocAICommandlet::CommandExecution::create(
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
UForbocAICommandlet::commandValidationPipeline() {
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
            TEXT("version"),          TEXT("status"),
            TEXT("doctor"),           TEXT("system_status"),
            TEXT("npc_list"),         TEXT("npc_create"),
            TEXT("npc_process"),      TEXT("npc_active"),
            TEXT("npc_state"),        TEXT("npc_update"),
            TEXT("npc_import"),       TEXT("npc_chat"),
            TEXT("memory_list"),      TEXT("memory_recall"),
            TEXT("memory_store"),     TEXT("memory_clear"),
            TEXT("memory_export"),
            TEXT("cortex_init"),      TEXT("cortex_init_remote"),
            TEXT("cortex_models"),    TEXT("cortex_complete"),
            TEXT("ghost_run"),        TEXT("ghost_status"),
            TEXT("ghost_results"),    TEXT("ghost_stop"),
            TEXT("ghost_history"),
            TEXT("bridge_validate"),  TEXT("bridge_rules"),
            TEXT("bridge_preset"),
            TEXT("rules_list"),       TEXT("rules_presets"),
            TEXT("rules_register"),   TEXT("rules_delete"),
            TEXT("soul_export"),      TEXT("soul_import"),
            TEXT("soul_import_npc"),  TEXT("soul_list"),
            TEXT("soul_verify"),
            TEXT("config_set"),       TEXT("config_get"),
            TEXT("config_list"),
            TEXT("vector_init")};

        if (!ValidCommands.Contains(Command)) {
          return CLITypes::make_left(
              FString::Printf(TEXT("Invalid command: %s"), *Command),
              FString());
        }
        return CLITypes::make_right(FString(), Command);
      });
}
