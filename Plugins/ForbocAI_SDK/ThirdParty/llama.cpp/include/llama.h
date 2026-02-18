#pragma once

// Placeholder for real llama.h from llama.cpp
namespace llama {
struct context;
context *load_model(const char *path);
void free_model(context *ctx);
const char *infer(context *ctx, const char *prompt, int max_tokens);
} // namespace llama
