#include "Ghost/GhostModule.h"
#include "Ghost/GhostModuleInternal.h"
#include "Ghost/GhostSlice.h"
#include "Ghost/GhostThunks.h"
#include "Core/functional_core.hpp"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NPC/NPCModule.h"
#include "RuntimeStore.h"
#include "Serialization/JsonSerializer.h"

// ==========================================================
// Ghost Module Implementation — Strict FP (Reduced)
// ==========================================================

FGhost GhostOps::Create(const FGhostConfig &Config) {
  FGhost Ghost;
  Ghost.Config = Config;
  Ghost.bInitialized = true;
  return Ghost;
}

// Single test — G.6: dispatch runLocalGhostTestThunk
GhostTypes::GhostTestRunResult GhostOps::RunTest(const FGhost &Ghost,
                                                 const FString &Scenario) {
  if (!Ghost.bInitialized) {
    return GhostTypes::AsyncResult<FGhostTestResult>::create(
        [](std::function<void(FGhostTestResult)>,
           std::function<void(std::string)> Reject) { Reject("Ghost not initialized"); });
  }
  auto Store = ConfigureStore();
  return Store.dispatch(rtk::runLocalGhostTestThunk(Ghost.Config.Agent, Scenario));
}

// All tests implementation (Async)
GhostTypes::GhostTestRunAllResult GhostOps::RunAllTests(const FGhost &Ghost) {
  return GhostTypes::AsyncResult<FGhostTestReport>::create(
      [Ghost](std::function<void(FGhostTestReport)> resolve,
              std::function<void(std::string)> reject) {
        auto Report = MakeShared<FGhostTestReport>();
        Report->Config = Ghost.Config;

        GhostInternal::RunTestsSequentially(
            Ghost, Report, 0,
            [resolve](FGhostTestReport FinalReport) {
              auto Store = ConfigureStore();
              Store.dispatch(
                  GhostSlice::Actions::GhostSessionCompleted(FinalReport));
              resolve(FinalReport);
            },
            [reject](std::string Error) {
              auto Store = ConfigureStore();
              Store.dispatch(GhostSlice::Actions::GhostSessionFailed(
                  TEXT("ghost-run"), FString(Error.c_str())));
              reject(Error);
            });
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
