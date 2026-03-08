#include "Ghost/GhostModule.h"
#include "NPC/NPCModule.h"
#include "Core/functional_core.hpp"
#include "Ghost/GhostModuleInternal.h"
#include "Ghost/GhostSlice.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SDKStore.h"
#include "Serialization/JsonSerializer.h"

// ==========================================================
// Ghost Module Implementation — Strict FP (Reduced)
// ==========================================================

// Single test implementation (Async)
GhostTypes::GhostTestRunResult GhostOps::RunTest(const FGhost &Ghost,
                                                 const FString &Scenario) {
  return GhostTypes::AsyncResult<FGhostTestResult>::create(
      [Ghost, Scenario](std::function<void(FGhostTestResult)> resolve,
                        std::function<void(std::string)> reject) {
        if (!Ghost.bInitialized) {
          reject("Ghost not initialized");
          return;
        }

        if (Scenario.IsEmpty()) {
          reject("Scenario cannot be empty");
          return;
        }

        auto SDKStore = ConfigureSDKStore();
        SDKStore.dispatch(GhostActions::GhostSessionStarted(Scenario));

        // Create agent from configuration
        FAgent Agent = AgentFactory::Create(Ghost.Config.Agent);

        // Run the scenario test via Internal helper
        GhostInternal::RunScenarioTest(Agent, Scenario)
            .then([resolve, SDKStore, Scenario](FGhostTestResult Result) {
              SDKStore.dispatch(GhostActions::GhostSessionCompleted(Result));
              resolve(Result);
            })
            .catch_([reject, SDKStore, Scenario](std::string Error) {
              FGhostTestResult Failure;
              Failure.Scenario = Scenario;
              Failure.bPassed = false;
              Failure.ErrorMessage = FString(Error.c_str());
              SDKStore.dispatch(GhostActions::GhostSessionCompleted(Failure));
              reject(Error);
            });
      });
}

// All tests implementation (Async)
GhostTypes::GhostTestRunAllResult GhostOps::RunAllTests(const FGhost &Ghost) {
  return GhostTypes::AsyncResult<FGhostTestReport>::create(
      [Ghost](std::function<void(FGhostTestReport)> resolve,
              std::function<void(std::string)> reject) {
        auto Report = MakeShared<FGhostTestReport>();
        Report->Config = Ghost.Config;

        GhostInternal::RunTestsSequentially(Ghost, Report, 0, resolve, reject);
      });
}

// Configuration validation implementation
GhostTypes::GhostValidationResult
GhostOps::ValidateConfig(const FGhostConfig &Config) {
  return GhostInternal::ValidateTestConfig(Config);
}

// Summary generation implementation
GhostTypes::Either<FString, FString>
GhostOps::GenerateSummary(const FGhostTestReport &Report) {
  return GhostInternal::GenerateTestSummary(Report);
}

// JSON export implementation
GhostTypes::Either<FString, FString>
GhostOps::ExportToJson(const FGhostTestReport &Report) {
  return GhostInternal::ExportResultsToJson(Report);
}

// CSV export implementation
GhostTypes::Either<FString, FString>
GhostOps::ExportToCsv(const FGhostTestReport &Report) {
  return GhostInternal::ExportResultsToCsv(Report);
}

// Functional helper implementations
namespace GhostHelpers {
GhostTypes::Lazy<FGhost> createLazyGhost(const FGhostConfig &config) {
  return func::lazy(
      [config]() -> FGhost { return GhostFactory::Create(config); });
}

GhostTypes::ValidationPipeline<FGhostConfig> ghostConfigValidationPipeline() {
  return func::validationPipeline<FGhostConfig>()
      .add([](const FGhostConfig &config)
               -> GhostTypes::Either<FString, FGhostConfig> {
        if (config.Agent.Id.IsEmpty() || config.Agent.Persona.IsEmpty()) {
          return GhostTypes::make_left(
              FString(TEXT("Agent must have valid Id and Persona")));
        }
        return GhostTypes::make_right(config);
      })
      .add([](const FGhostConfig &config)
               -> GhostTypes::Either<FString, FGhostConfig> {
        if (config.Scenarios.Num() == 0) {
          return GhostTypes::make_left(
              FString(TEXT("At least one test scenario must be provided")));
        }
        return GhostTypes::make_right(config);
      })
      .add([](const FGhostConfig &config)
               -> GhostTypes::Either<FString, FGhostConfig> {
        if (config.MaxIterations < 1) {
          return GhostTypes::make_left(
              FString(TEXT("Max iterations must be at least 1")));
        }
        return GhostTypes::make_right(config);
      });
}

GhostTypes::Pipeline<FGhost> ghostTestPipeline(const FGhost &ghost) {
  return func::pipe(ghost);
}

GhostTypes::Curried<
    1, std::function<GhostTypes::GhostCreationResult(FGhostConfig)>>
curriedGhostCreation() {
  return func::curry<1>(
      [](FGhostConfig config) -> GhostTypes::GhostCreationResult {
        try {
          FGhost ghost = GhostFactory::Create(config);
          return GhostTypes::make_right(FString(), ghost);
        } catch (const std::exception &e) {
          return GhostTypes::make_left(FString(e.what()));
        }
      });
}
} // namespace GhostHelpers