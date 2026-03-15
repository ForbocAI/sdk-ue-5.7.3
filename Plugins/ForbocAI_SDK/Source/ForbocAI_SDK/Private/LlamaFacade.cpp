/**
 * LlamaFacade — Wraps llama.cpp C API for inference and embeddings
 * When WITH_FORBOC_NATIVE=1: Requires real libllama.a and llama.cpp include
 * folder. API targets llama.cpp master (b5220+). Batch/sampler API may need
 * adjustment for your llama.cpp version.
 * When WITH_FORBOC_NATIVE=0: Stub implementation returns nullptr/false.
 * User Story: As a maintainer, I need this note so the surrounding API intent stays clear during maintenance and integration.
 */

#include "LlamaFacade.h"
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#if WITH_FORBOC_NATIVE

#include "llama.h"

struct llama_facade_context {
  llama_model *Model;
  llama_context *Ctx;
  bool IsEmbedding;
};

static void NormalizeVector(float *Data, int N) {
  float SumSq = 0.0f;
  for (int i = 0; i < N; ++i) {
    SumSq += Data[i] * Data[i];
  }
  if (SumSq <= 1e-10f) return;
  float InvNorm = 1.0f / std::sqrt(SumSq);
  for (int i = 0; i < N; ++i) {
    Data[i] *= InvNorm;
  }
}

namespace LlamaFacade {

static bool BackendInitialized = false;

llama_facade_context *LoadInferenceModel(const char *PathUtf8) {
  if (!PathUtf8 || !*PathUtf8) return nullptr;
  if (!BackendInitialized) {
    llama_backend_init();
    BackendInitialized = true;
  }

  llama_model_params MParams = llama_model_default_params();
  MParams.n_gpu_layers = -1;

  llama_model *Model = llama_model_load_from_file(PathUtf8, MParams);
  if (!Model) return nullptr;

  llama_context_params CParams = llama_context_default_params();
  CParams.n_ctx = 2048;
  CParams.n_batch = 512;
  CParams.embeddings = false;

  llama_context *Ctx = llama_init_from_model(Model, CParams);
  if (!Ctx) {
    llama_model_free(Model);
    return nullptr;
  }

  std::unique_ptr<llama_facade_context> F(
      new llama_facade_context{Model, Ctx, false});
  return F.release(); // Ownership transferred to caller; freed via FreeContext
}

llama_facade_context *LoadEmbeddingModel(const char *PathUtf8) {
  if (!PathUtf8 || !*PathUtf8) return nullptr;
  if (!BackendInitialized) {
    llama_backend_init();
    BackendInitialized = true;
  }

  llama_model_params MParams = llama_model_default_params();
  MParams.n_gpu_layers = -1;

  llama_model *Model = llama_model_load_from_file(PathUtf8, MParams);
  if (!Model) return nullptr;

  llama_context_params CParams = llama_context_default_params();
  CParams.n_ctx = 512;
  CParams.n_batch = 512;
  CParams.embeddings = true;
  CParams.pooling_type = LLAMA_POOLING_TYPE_MEAN;

  llama_context *Ctx = llama_init_from_model(Model, CParams);
  if (!Ctx) {
    llama_model_free(Model);
    return nullptr;
  }

  std::unique_ptr<llama_facade_context> F(
      new llama_facade_context{Model, Ctx, true});
  return F.release(); // Ownership transferred to caller; freed via FreeContext
}

void FreeContext(llama_facade_context *Ctx) {
  if (!Ctx) return;
  llama_free(Ctx->Ctx);
  llama_model_free(Ctx->Model);
  std::unique_ptr<llama_facade_context> Guard(Ctx);
  // Guard destructor calls delete
}

char *Infer(llama_facade_context *Ctx, const char *PromptUtf8, int MaxTokens,
            float Temperature) {
  if (!Ctx || !PromptUtf8 || Ctx->IsEmbedding) return nullptr;

  const llama_vocab *Vocab = llama_model_get_vocab(Ctx->Model);
  const int32_t CtxSize = llama_n_ctx(Ctx->Ctx);

  std::vector<llama_token> Tokens(CtxSize, 0);
  int32_t N = llama_tokenize(Vocab, PromptUtf8, -1, Tokens.data(),
                             static_cast<int32_t>(Tokens.size()), true, false);
  if (N < 0) return nullptr;
  Tokens.resize(static_cast<size_t>(N));

  llama_sampler_chain_params SParams = llama_sampler_chain_default_params();
  llama_sampler *Smpl = llama_sampler_chain_init(SParams);
  llama_sampler_chain_add(Smpl, llama_sampler_init_top_k(40));
  llama_sampler_chain_add(Smpl, llama_sampler_init_top_p(0.9f, 1));
  llama_sampler_chain_add(Smpl, llama_sampler_init_temp(Temperature));
  llama_sampler_chain_add(Smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

  llama_batch Batch = llama_batch_init(512, 0, 1);

  int32_t Pos = 0;
  for (size_t i = 0; i < Tokens.size(); ++i) {
    llama_batch_add(Batch, Tokens[i], Pos, {0}, true);
    ++Pos;
  }
  if (llama_decode(Ctx->Ctx, Batch) != 0) {
    llama_batch_free(Batch);
    llama_sampler_free(Smpl);
    return nullptr;
  }

  std::string Output;
  int Generated = 0;
  while (Generated < MaxTokens) {
    llama_token Next =
        llama_sampler_sample(Smpl, Ctx->Ctx, -1);
    if (Next == llama_vocab_eos(Vocab)) break;

    char Buf[256];
    int Len = llama_token_to_piece(Vocab, Next, Buf, sizeof(Buf), 0, false);
    if (Len > 0) {
      Output.append(Buf, static_cast<size_t>(Len));
    }
    llama_batch_clear(Batch);
    llama_batch_add(Batch, Next, Pos, {0}, true);
    ++Pos;
    ++Generated;
    if (llama_decode(Ctx->Ctx, Batch) != 0) break;
  }

  llama_batch_free(Batch);
  llama_sampler_free(Smpl);

  char *Result = static_cast<char *>(malloc(Output.size() + 1));
  if (Result) {
    memcpy(Result, Output.c_str(), Output.size() + 1);
  }
  return Result;
}

int InferStream(llama_facade_context *Ctx, const char *PromptUtf8, int MaxTokens,
                float Temperature, TokenCallback OnToken, void *UserData) {
  if (!Ctx || !PromptUtf8 || Ctx->IsEmbedding || !OnToken) return 0;

  const llama_vocab *Vocab = llama_model_get_vocab(Ctx->Model);
  const int32_t CtxSize = llama_n_ctx(Ctx->Ctx);

  std::vector<llama_token> Tokens(CtxSize, 0);
  int32_t N = llama_tokenize(Vocab, PromptUtf8, -1, Tokens.data(),
                             static_cast<int32_t>(Tokens.size()), true, false);
  if (N < 0) return 0;
  Tokens.resize(static_cast<size_t>(N));

  llama_sampler_chain_params SParams = llama_sampler_chain_default_params();
  llama_sampler *Smpl = llama_sampler_chain_init(SParams);
  llama_sampler_chain_add(Smpl, llama_sampler_init_top_k(40));
  llama_sampler_chain_add(Smpl, llama_sampler_init_top_p(0.9f, 1));
  llama_sampler_chain_add(Smpl, llama_sampler_init_temp(Temperature));
  llama_sampler_chain_add(Smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

  llama_batch Batch = llama_batch_init(512, 0, 1);

  int32_t Pos = 0;
  for (size_t i = 0; i < Tokens.size(); ++i) {
    llama_batch_add(Batch, Tokens[i], Pos, {0}, true);
    ++Pos;
  }
  if (llama_decode(Ctx->Ctx, Batch) != 0) {
    llama_batch_free(Batch);
    llama_sampler_free(Smpl);
    return 0;
  }

  int Generated = 0;
  while (Generated < MaxTokens) {
    llama_token Next = llama_sampler_sample(Smpl, Ctx->Ctx, -1);
    if (Next == llama_vocab_eos(Vocab)) break;

    char Buf[256];
    int Len = llama_token_to_piece(Vocab, Next, Buf, sizeof(Buf), 0, false);
    if (Len > 0) {
      OnToken(Buf, Len, UserData);
    }
    llama_batch_clear(Batch);
    llama_batch_add(Batch, Next, Pos, {0}, true);
    ++Pos;
    ++Generated;
    if (llama_decode(Ctx->Ctx, Batch) != 0) break;
  }

  llama_batch_free(Batch);
  llama_sampler_free(Smpl);
  return Generated;
}

char *InferWithGrammar(llama_facade_context *Ctx, const char *PromptUtf8,
                       int MaxTokens, float Temperature,
                       const char *GrammarUtf8) {
  if (!Ctx || !PromptUtf8 || Ctx->IsEmbedding) return nullptr;

  /**
   * If no grammar provided, fall back to regular Infer
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  if (!GrammarUtf8 || !*GrammarUtf8) {
    return Infer(Ctx, PromptUtf8, MaxTokens, Temperature);
  }

  const llama_vocab *Vocab = llama_model_get_vocab(Ctx->Model);
  const int32_t CtxSize = llama_n_ctx(Ctx->Ctx);

  std::vector<llama_token> Tokens(CtxSize, 0);
  int32_t N = llama_tokenize(Vocab, PromptUtf8, -1, Tokens.data(),
                             static_cast<int32_t>(Tokens.size()), true, false);
  if (N < 0) return nullptr;
  Tokens.resize(static_cast<size_t>(N));

  llama_sampler_chain_params SParams = llama_sampler_chain_default_params();
  llama_sampler *Smpl = llama_sampler_chain_init(SParams);
  llama_sampler_chain_add(Smpl, llama_sampler_init_top_k(40));
  llama_sampler_chain_add(Smpl, llama_sampler_init_top_p(0.9f, 1));
  llama_sampler_chain_add(Smpl, llama_sampler_init_temp(Temperature));
  /**
   * G11: Add GBNF grammar sampler for constrained output
   * User Story: As a maintainer, I need this step note so I can follow the scenario progression and reason about the expected state changes.
   */
  llama_sampler_chain_add(Smpl,
      llama_sampler_init_grammar(Vocab, GrammarUtf8, "root"));
  llama_sampler_chain_add(Smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

  llama_batch Batch = llama_batch_init(512, 0, 1);

  int32_t Pos = 0;
  for (size_t i = 0; i < Tokens.size(); ++i) {
    llama_batch_add(Batch, Tokens[i], Pos, {0}, true);
    ++Pos;
  }
  if (llama_decode(Ctx->Ctx, Batch) != 0) {
    llama_batch_free(Batch);
    llama_sampler_free(Smpl);
    return nullptr;
  }

  std::string Output;
  int Generated = 0;
  while (Generated < MaxTokens) {
    llama_token Next =
        llama_sampler_sample(Smpl, Ctx->Ctx, -1);
    if (Next == llama_vocab_eos(Vocab)) break;

    char Buf[256];
    int Len = llama_token_to_piece(Vocab, Next, Buf, sizeof(Buf), 0, false);
    if (Len > 0) {
      Output.append(Buf, static_cast<size_t>(Len));
    }
    llama_batch_clear(Batch);
    llama_batch_add(Batch, Next, Pos, {0}, true);
    ++Pos;
    ++Generated;
    if (llama_decode(Ctx->Ctx, Batch) != 0) break;
  }

  llama_batch_free(Batch);
  llama_sampler_free(Smpl);

  char *Result = static_cast<char *>(malloc(Output.size() + 1));
  if (Result) {
    memcpy(Result, Output.c_str(), Output.size() + 1);
  }
  return Result;
}

bool Embed(llama_facade_context *Ctx, const char *TextUtf8, float *Out,
           int Dims) {
  if (!Ctx || !TextUtf8 || !Out || Dims < 1 || !Ctx->IsEmbedding) return false;

  const llama_vocab *Vocab = llama_model_get_vocab(Ctx->Model);
  std::vector<llama_token> Tokens(512, 0);
  int32_t N = llama_tokenize(Vocab, TextUtf8, -1, Tokens.data(),
                             static_cast<int32_t>(Tokens.size()), true, false);
  if (N <= 0) return false;
  Tokens.resize(static_cast<size_t>(N));

  llama_batch Batch = llama_batch_init(512, 0, 1);
  for (size_t i = 0; i < Tokens.size(); ++i) {
    llama_batch_add(Batch, Tokens[i], static_cast<int32_t>(i), {0}, true);
  }

  llama_memory_clear(llama_get_memory(Ctx->Ctx), true);
  if (llama_decode(Ctx->Ctx, Batch) != 0) {
    llama_batch_free(Batch);
    return false;
  }

  const int32_t NEmb = llama_model_n_embd_out(Ctx->Model);
  const float *Emb = llama_get_embeddings_seq(Ctx->Ctx, 0);
  if (!Emb) {
    llama_batch_free(Batch);
    return false;
  }

  const int Copy = Dims < NEmb ? Dims : NEmb;
  for (int i = 0; i < Copy; ++i) Out[i] = Emb[i];
  for (int i = Copy; i < Dims; ++i) Out[i] = 0.0f;
  NormalizeVector(Out, Dims);

  llama_batch_free(Batch);
  return true;
}

} // namespace LlamaFacade

#else

namespace LlamaFacade {

llama_facade_context *LoadInferenceModel(const char *) { return nullptr; }
llama_facade_context *LoadEmbeddingModel(const char *) { return nullptr; }
void FreeContext(llama_facade_context *) {}
char *Infer(llama_facade_context *, const char *, int, float) { return nullptr; }
char *InferWithGrammar(llama_facade_context *, const char *, int, float, const char *) { return nullptr; }
int InferStream(llama_facade_context *, const char *, int, float, TokenCallback, void *) { return 0; }
bool Embed(llama_facade_context *, const char *, float *, int) { return false; }

} // namespace LlamaFacade

#endif
