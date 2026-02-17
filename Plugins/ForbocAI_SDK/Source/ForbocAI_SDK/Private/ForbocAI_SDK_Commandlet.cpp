#include "ForbocAI_SDK_Commandlet.h"
#include "CLIModule.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Core/functional_core.hpp"

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

  // Command Routing with functional patterns
  auto commandResult = createCommandPipeline(Command, ApiUrl, Params);
  commandResult.execute();

  // HTTP requests are now handled synchronously via SendRequestAndWait
  // in CLIModule, which pumps the HTTP tick loop until completion.

  return 0;
}

// Command execution helpers
CLITypes::CommandResult UForbocAI_SDKCommandlet::executeDoctor(const FString& apiUrl) {
    return CLIOps::Doctor(apiUrl);
}

CLITypes::CommandResult UForbocAI_SDKCommandlet::executeAgentList(const FString& apiUrl) {
    return CLIOps::ListAgents(apiUrl);
}

CLITypes::CommandResult UForbocAI_SDKCommandlet::executeAgentCreate(const FString& apiUrl, const FString& persona) {
    return CLIOps::CreateAgent(apiUrl, persona);
}

CLITypes::CommandResult UForbocAI_SDKCommandlet::executeAgentProcess(const FString& apiUrl, const FString& id, const FString& input) {
    return CLIOps::ProcessAgent(apiUrl, id, input);
}

CLITypes::CommandResult UForbocAI_SDKCommandlet::executeSoulExport(const FString& apiUrl, const FString& id) {
    return CLIOps::ExportSoul(apiUrl, id);
}

// Command pipeline helpers
CLITypes::CommandExecution UForbocAI_SDKCommandlet::createCommandPipeline(const FString& command, const FString& apiUrl, const FString& param1, const FString& param2) {
    return CLITypes::CommandExecution::create([this, command, apiUrl, param1, param2](auto resolve, auto reject) {
        if (command == TEXT("doctor")) {
            auto result = executeDoctor(apiUrl);
            if (result.isSuccessful()) {
                resolve();
            } else {
                reject(result.message);
            }
        } else if (command == TEXT("agent_list")) {
            auto result = executeAgentList(apiUrl);
            if (result.isSuccessful()) {
                resolve();
            } else {
                reject(result.message);
            }
        } else if (command == TEXT("agent_create")) {
            FString persona;
            FParse::Value(*param1, TEXT("Persona="), persona);
            if (persona.IsEmpty()) {
                persona = TEXT("Default UE Persona");
            }
            auto result = executeAgentCreate(apiUrl, persona);
            if (result.isSuccessful()) {
                resolve();
            } else {
                reject(result.message);
            }
        } else if (command == TEXT("agent_process")) {
            FString id, input;
            FParse::Value(*param1, TEXT("Id="), id);
            FParse::Value(*param2, TEXT("Input="), input);

            if (!id.IsEmpty() && !input.IsEmpty()) {
                auto result = executeAgentProcess(apiUrl, id, input);
                if (result.isSuccessful()) {
                    resolve();
                } else {
                    reject(result.message);
                }
            } else {
                FString error = FString::Printf(TEXT("Missing Id or Input for agent_process: Id=%s, Input=%s"), *id, *input);
                reject(error);
            }
        } else if (command == TEXT("soul_export")) {
            FString id;
            FParse::Value(*param1, TEXT("Id="), id);

            if (!id.IsEmpty()) {
                auto result = executeSoulExport(apiUrl, id);
                if (result.isSuccessful()) {
                    resolve();
                } else {
                    reject(result.message);
                }
            } else {
                reject(FString(TEXT("Missing Id for soul_export")));
            }
        } else {
            FString error = FString::Printf(TEXT("Unknown Command or No Command Specified. usage: -Command=doctor"));
            reject(error);
        }
    });
}

// Command validation helpers
CLITypes::ValidationPipeline<FString> UForbocAI_SDKCommandlet::commandValidationPipeline() {
    return CLITypes::ValidationPipeline<FString>()
        .add([](const FString& command) -> CLITypes::Either<FString, FString> {
            if (command.IsEmpty()) {
                return CLITypes::make_left(FString(TEXT("Command cannot be empty")));
            }
            return CLITypes::make_right(command);
        })
        .add([](const FString& command) -> CLITypes::Either<FString, FString> {
            static const TArray<FString> validCommands = {
                TEXT("doctor"),
                TEXT("agent_list"),
                TEXT("agent_create"),
                TEXT("agent_process"),
                TEXT("soul_export")
            };
            
            if (!validCommands.Contains(command)) {
                return CLITypes::make_left(FString::Printf(TEXT("Invalid command: %s"), *command));
            }
            return CLITypes::make_right(command);
        });
}