#pragma once

/**
 * LlamaFacade — Thin wrapper over llama.cpp C API
 *
 * Provides a simple interface for inference and embeddings. Used by NativeEngine
 * when WITH_FORBOC_NATIVE=1. Opaque to avoid pulling in llama headers elsewhere.
 */

struct llama_facade_context;

namespace LlamaFacade {

/** Load model for inference (SmolLM, Llama3, etc.) */
llama_facade_context *LoadInferenceModel(const char *PathUtf8);

/** Load embedding model (all-MiniLM-L6-v2 GGUF). Use for Embed(). */
llama_facade_context *LoadEmbeddingModel(const char *PathUtf8);

void FreeContext(llama_facade_context *Ctx);

/** Generate text. Returns allocated UTF-8 string; caller must free. */
char *Infer(llama_facade_context *Ctx, const char *PromptUtf8, int MaxTokens,
            float Temperature);

/** Token callback for streaming inference. */
typedef void (*TokenCallback)(const char *TokenUtf8, int Len, void *UserData);

/** Generate text token-by-token, calling OnToken for each. Returns generated count. */
int InferStream(llama_facade_context *Ctx, const char *PromptUtf8, int MaxTokens,
                float Temperature, TokenCallback OnToken, void *UserData);

/** G11: Generate text with GBNF grammar constraint. Returns allocated UTF-8 string; caller must free.
 *  GrammarUtf8 is a GBNF grammar string. If null or empty, behaves like Infer(). */
char *InferWithGrammar(llama_facade_context *Ctx, const char *PromptUtf8,
                       int MaxTokens, float Temperature,
                       const char *GrammarUtf8);

/** Generate 384-dim normalized embedding. Caller provides Out[384]. */
bool Embed(llama_facade_context *Ctx, const char *TextUtf8, float *Out, int Dims);

} // namespace LlamaFacade
