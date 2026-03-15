#pragma once
// Static scenario definitions — mirrors TS test-game scenarioSlice steps
// 4 scenarios × N commands = full CLI coverage matrix

#include "CoreMinimal.h"
#include "TestGame/TestGameTypes.h"

namespace TestGame {

inline TArray<FScenarioStep> GetDefaultScenarioSteps() {
  TArray<FScenarioStep> Steps;

  // --- Scenario 1: Spatial Strategy & Stealth ---
  {
    FScenarioStep S;
    S.Id = TEXT("stealth-door-open");
    S.Title = TEXT("Spatial Strategy & Stealth");
    S.Description = TEXT("Armory door is left open. Doomguard patrol "
                         "processes observation and stores memory.");
    S.EventType = EEventType::Stealth;

    S.Commands.Add({ECommandGroup::Status,
                    TEXT("forbocai status"),
                    {TEXT("GET /status")}});
    S.Commands.Add({ECommandGroup::NpcLifecycle,
                    TEXT("forbocai npc create doomguard"),
                    {TEXT("local only")}});
    S.Commands.Add({ECommandGroup::NpcLifecycle,
                    TEXT("forbocai npc state doomguard"),
                    {TEXT("local only")}});
    S.Commands.Add(
        {ECommandGroup::NpcLifecycle,
         TEXT("forbocai npc update doomguard faction Doomguards"),
         {TEXT("local only")}});
    S.Commands.Add(
        {ECommandGroup::NpcProcessChat,
         TEXT("forbocai npc process doomguard \"The armory door is open\""),
         {TEXT("POST /npcs/{id}/directive"), TEXT("POST /npcs/{id}/context"),
          TEXT("POST /npcs/{id}/verdict")}});
    S.Commands.Add(
        {ECommandGroup::MemoryStore,
         TEXT("forbocai memory store doomguard \"Armory door found open at "
              "x:5, y:12\""),
         {TEXT("POST /npcs/{id}/memory")}});
    S.Commands.Add({ECommandGroup::MemoryList,
                    TEXT("forbocai memory list doomguard"),
                    {TEXT("GET /npcs/{id}/memory")}});
    S.Commands.Add({ECommandGroup::BridgeRules,
                    TEXT("forbocai bridge rules"),
                    {TEXT("GET /bridge/rules")}});

    Steps.Add(S);
  }

  // --- Scenario 2: Social Simulation ---
  {
    FScenarioStep S;
    S.Id = TEXT("social-miller-encounter");
    S.Title = TEXT("Social Simulation");
    S.Description =
        TEXT("Miller encounter triggers recall, dialogue, and trade offer.");
    S.EventType = EEventType::Social;

    S.Commands.Add({ECommandGroup::NpcLifecycle,
                    TEXT("forbocai npc create miller"),
                    {TEXT("local only")}});
    S.Commands.Add(
        {ECommandGroup::NpcProcessChat,
         TEXT("forbocai npc chat miller --text \"I come in peace\""),
         {TEXT("POST /npcs/{id}/directive"), TEXT("POST /npcs/{id}/context"),
          TEXT("POST /npcs/{id}/verdict")}});
    S.Commands.Add(
        {ECommandGroup::MemoryRecall,
         TEXT("forbocai memory recall miller \"player interaction\""),
         {TEXT("POST /npcs/{id}/memory/recall")}});
    S.Commands.Add({ECommandGroup::BridgePreset,
                    TEXT("forbocai bridge preset social"),
                    {TEXT("POST /rules/presets/{id}")}});
    S.Commands.Add(
        {ECommandGroup::SoulExport, TEXT("forbocai soul export miller"),
         {TEXT("POST /npcs/{id}/soul/export"),
          TEXT("POST /npcs/{id}/soul/confirm")}});
    S.Commands.Add({ECommandGroup::SoulList,
                    TEXT("forbocai soul list"),
                    {TEXT("GET /souls")}});

    Steps.Add(S);
  }

  // --- Scenario 3: Real-Time Escape ---
  {
    FScenarioStep S;
    S.Id = TEXT("escape-realtime-pursuit");
    S.Title = TEXT("Real-Time Escape");
    S.Description = TEXT("Harness loop fires process commands and validates "
                         "jump force via bridge rules.");
    S.EventType = EEventType::Escape;

    S.Commands.Add(
        {ECommandGroup::BridgeValidate,
         TEXT("forbocai bridge validate doomguard-jump"),
         {TEXT("POST /bridge/validate"),
          TEXT("POST /bridge/validate/{id}")}});
    S.Commands.Add(
        {ECommandGroup::NpcProcessChat,
         TEXT("forbocai npc process doomguard \"Player is escaping\""),
         {TEXT("POST /npcs/{id}/directive"), TEXT("POST /npcs/{id}/context"),
          TEXT("POST /npcs/{id}/verdict")}});
    S.Commands.Add(
        {ECommandGroup::NpcProcessChat,
         TEXT("forbocai npc process doomguard \"Player jumped the gap\""),
         {TEXT("POST /npcs/{id}/directive"), TEXT("POST /npcs/{id}/context"),
          TEXT("POST /npcs/{id}/verdict")}});
    S.Commands.Add(
        {ECommandGroup::NpcProcessChat,
         TEXT("forbocai npc process doomguard \"Player reached the gate\""),
         {TEXT("POST /npcs/{id}/directive"), TEXT("POST /npcs/{id}/context"),
          TEXT("POST /npcs/{id}/verdict")}});
    S.Commands.Add({ECommandGroup::GhostLifecycle,
                    TEXT("forbocai ghost run smoke"),
                    {TEXT("POST /ghost/run")}});
    S.Commands.Add({ECommandGroup::GhostLifecycle,
                    TEXT("forbocai ghost status ghost-001"),
                    {TEXT("GET /ghost/{id}/status")}});
    S.Commands.Add({ECommandGroup::GhostLifecycle,
                    TEXT("forbocai ghost results ghost-001"),
                    {TEXT("GET /ghost/{id}/results")}});
    S.Commands.Add({ECommandGroup::GhostLifecycle,
                    TEXT("forbocai ghost stop ghost-001"),
                    {TEXT("POST /ghost/{id}/stop")}});
    S.Commands.Add({ECommandGroup::GhostLifecycle,
                    TEXT("forbocai ghost history"),
                    {TEXT("GET /ghost/history")}});

    Steps.Add(S);
  }

  // --- Scenario 4: Persistence & Recovery ---
  {
    FScenarioStep S;
    S.Id = TEXT("persistence-recovery");
    S.Title = TEXT("Persistence & Recovery");
    S.Description = TEXT("Exercises memory export/clear and soul import/chat "
                         "continuity.");
    S.EventType = EEventType::Persistence;

    S.Commands.Add({ECommandGroup::MemoryExport,
                    TEXT("forbocai memory export doomguard"),
                    {TEXT("local only")}});
    S.Commands.Add({ECommandGroup::MemoryClear,
                    TEXT("forbocai memory clear doomguard"),
                    {TEXT("DELETE /npcs/{id}/memory/clear")}});
    S.Commands.Add(
        {ECommandGroup::SoulImport,
         TEXT("forbocai soul import tx-demo-001"),
         {TEXT("GET /souls/{txId}"), TEXT("POST /npcs/import"),
          TEXT("POST /npcs/import/confirm")}});
    S.Commands.Add(
        {ECommandGroup::SoulChat,
         TEXT("forbocai soul chat doomguard --text \"What do you remember?\""),
         {TEXT("POST /npcs/{id}/directive"), TEXT("POST /npcs/{id}/context"),
          TEXT("POST /npcs/{id}/verdict")}});
    S.Commands.Add({ECommandGroup::CortexInit,
                    TEXT("forbocai cortex init --remote"),
                    {TEXT("POST /cortex/init")}});

    Steps.Add(S);
  }

  return Steps;
}

} // namespace TestGame
