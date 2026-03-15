// Tests for G7 streaming completions — CortexSlice actions + NativeEngine

#include "Cortex/CortexSlice.h"
#include "Core/functional_core.hpp"
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "NativeEngine.h"

// ---------------------------------------------------------------------------
// Test: CortexStreamStart clears state and sets bIsStreaming
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCortexStreamStartTest,
    "ForbocAI.Cortex.Stream.StartAction",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::EngineFilter)
bool FCortexStreamStartTest::RunTest(const FString &Parameters) {
  auto Slice = CortexSlice::CreateCortexSlice();
  auto State = Slice.initialState;

  // Simulate some prior state
  State.LastResponseText = TEXT("old response");
  State.Error = TEXT("old error");

  auto Action = CortexSlice::Actions::CortexStreamStart(TEXT("test prompt"));
  State = Slice.reducer(State, Action);

  TestTrue("bIsStreaming is true after StreamStart", State.bIsStreaming);
  TestTrue("StreamAccumulated is empty", State.StreamAccumulated.IsEmpty());
  TestEqual("LastPrompt set", State.LastPrompt, TEXT("test prompt"));
  TestTrue("Error cleared", State.Error.IsEmpty());

  return true;
}

// ---------------------------------------------------------------------------
// Test: CortexStreamToken accumulates text
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCortexStreamTokenTest,
    "ForbocAI.Cortex.Stream.TokenAction",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::EngineFilter)
bool FCortexStreamTokenTest::RunTest(const FString &Parameters) {
  auto Slice = CortexSlice::CreateCortexSlice();
  auto State = Slice.initialState;

  // Start streaming
  State = Slice.reducer(State, CortexSlice::Actions::CortexStreamStart(TEXT("p")));

  // Send tokens
  State = Slice.reducer(State, CortexSlice::Actions::CortexStreamToken(TEXT("Hello")));
  TestEqual("First token accumulated", State.StreamAccumulated, TEXT("Hello"));

  State = Slice.reducer(State, CortexSlice::Actions::CortexStreamToken(TEXT(" world")));
  TestEqual("Second token accumulated", State.StreamAccumulated, TEXT("Hello world"));

  State = Slice.reducer(State, CortexSlice::Actions::CortexStreamToken(TEXT("!")));
  TestEqual("Third token accumulated", State.StreamAccumulated, TEXT("Hello world!"));

  TestTrue("Still streaming", State.bIsStreaming);

  return true;
}

// ---------------------------------------------------------------------------
// Test: CortexStreamComplete sets final text and clears streaming state
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCortexStreamCompleteTest,
    "ForbocAI.Cortex.Stream.CompleteAction",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::EngineFilter)
bool FCortexStreamCompleteTest::RunTest(const FString &Parameters) {
  auto Slice = CortexSlice::CreateCortexSlice();
  auto State = Slice.initialState;

  // Start + tokens + complete
  State = Slice.reducer(State, CortexSlice::Actions::CortexStreamStart(TEXT("p")));
  State = Slice.reducer(State, CortexSlice::Actions::CortexStreamToken(TEXT("Hello")));
  State = Slice.reducer(State, CortexSlice::Actions::CortexStreamComplete(TEXT("Hello world")));

  TestFalse("bIsStreaming is false after complete", State.bIsStreaming);
  TestEqual("LastResponseText set to full text", State.LastResponseText, TEXT("Hello world"));
  TestTrue("StreamAccumulated cleared", State.StreamAccumulated.IsEmpty());

  return true;
}

// ---------------------------------------------------------------------------
// Test: LoadModel returns nullptr for non-existent model path
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadModelNonExistentTest,
    "ForbocAI.Cortex.Stream.LoadModelNonExistent",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::EngineFilter)
bool FLoadModelNonExistentTest::RunTest(const FString &Parameters) {
  Native::Llama::Context Ctx =
      Native::Llama::LoadModel(TEXT("non_existent_model.bin"));

  TestTrue("LoadModel returns nullptr for non-existent file", Ctx == nullptr);

  FCortexConfig Config;
  Config.MaxTokens = 64;

  TArray<FString> ReceivedTokens;
  FString Result = Native::Llama::InferStream(
      Ctx, TEXT("Hello NPC"), Config,
      [&ReceivedTokens](const FString &Token) {
        ReceivedTokens.Add(Token);
      });

  TestEqual("No tokens received for null context", ReceivedTokens.Num(), 0);
  TestTrue("Returns error string", Result.Contains(TEXT("Error")));

  return true;
}

// ---------------------------------------------------------------------------
// Test: InferStream with null context returns error
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FInferStreamNullCtxTest,
    "ForbocAI.Cortex.Stream.NullContext",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::EngineFilter)
bool FInferStreamNullCtxTest::RunTest(const FString &Parameters) {
  FCortexConfig Config;
  Config.MaxTokens = 64;

  TArray<FString> ReceivedTokens;
  FString Result = Native::Llama::InferStream(
      nullptr, TEXT("Should fail"), Config,
      [&ReceivedTokens](const FString &Token) {
        ReceivedTokens.Add(Token);
      });

  TestEqual("No tokens received for null context", ReceivedTokens.Num(), 0);
  TestTrue("Returns error string", Result.Contains(TEXT("Error")));

  return true;
}
