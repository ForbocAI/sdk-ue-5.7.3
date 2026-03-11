/**
 * llama.h — llama.cpp C API header
 *
 * Targets llama.cpp master branch b5220+ API surface.
 * This header declares the subset of the llama.cpp public API used by
 * ForbocAI_SDK/Private/LlamaFacade.cpp. The actual implementation is
 * provided by linking against libllama.a (or libllama.so/dylib).
 *
 * Reference: https://github.com/ggml-org/llama.cpp
 */

#ifndef LLAMA_H
#define LLAMA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
#  include <initializer_list>
extern "C" {
#endif

/*
 * ----------------------------------------------------------------------------
 *  Constants
 * ----------------------------------------------------------------------------
 */

#define LLAMA_DEFAULT_SEED 0xFFFFFFFF

/*
 * ----------------------------------------------------------------------------
 *  Basic typedefs
 * ----------------------------------------------------------------------------
 */

typedef int32_t llama_token;
typedef int32_t llama_pos;
typedef int32_t llama_seq_id;

/*
 * ----------------------------------------------------------------------------
 *  Opaque types (forward declarations)
 * ----------------------------------------------------------------------------
 */

struct llama_model;
struct llama_context;
struct llama_vocab;
struct llama_sampler;
struct llama_memory;

/*
 * ----------------------------------------------------------------------------
 *  Enums
 * ----------------------------------------------------------------------------
 */

enum llama_pooling_type {
    LLAMA_POOLING_TYPE_UNSPECIFIED = -1,
    LLAMA_POOLING_TYPE_NONE       =  0,
    LLAMA_POOLING_TYPE_MEAN       =  1,
    LLAMA_POOLING_TYPE_CLS        =  2,
    LLAMA_POOLING_TYPE_LAST       =  3,
    LLAMA_POOLING_TYPE_RANK       =  4,
};

/*
 * ----------------------------------------------------------------------------
 *  Parameter structs
 * ----------------------------------------------------------------------------
 */

struct llama_model_params {
    int32_t n_gpu_layers;

    /* Padding / future fields — the real struct has many more members.
     * We only expose the field(s) LlamaFacade.cpp actually touches.
     * When linking against the real libllama the full struct layout is
     * provided by the library's own header; this file is used only when
     * the plugin is compiled in header-only / stub mode.  The default-
     * params factory function returns a correctly-sized, zero-initialised
     * struct regardless. */
};

struct llama_context_params {
    uint32_t n_ctx;
    uint32_t n_batch;
    bool     embeddings;
    enum llama_pooling_type pooling_type;

    /* See note on llama_model_params above. */
};

struct llama_sampler_chain_params {
    bool no_perf;  /* disable performance counters */
};

/*
 * ----------------------------------------------------------------------------
 *  Batch
 * ----------------------------------------------------------------------------
 */

struct llama_batch {
    int32_t       n_tokens;

    llama_token  * token;
    float        * embd;
    llama_pos    * pos;
    int32_t      * n_seq_id;
    llama_seq_id ** seq_id;
    int8_t       * logits;
};

/*
 * ----------------------------------------------------------------------------
 *  Backend lifetime
 * ----------------------------------------------------------------------------
 */

void llama_backend_init(void);
void llama_backend_free(void);

/*
 * ----------------------------------------------------------------------------
 *  Model
 * ----------------------------------------------------------------------------
 */

struct llama_model_params llama_model_default_params(void);

struct llama_model * llama_model_load_from_file(
        const char               * path_model,
        struct llama_model_params   params);

void llama_model_free(struct llama_model * model);

const struct llama_vocab * llama_model_get_vocab(const struct llama_model * model);

int32_t llama_model_n_embd_out(const struct llama_model * model);

/*
 * ----------------------------------------------------------------------------
 *  Context
 * ----------------------------------------------------------------------------
 */

struct llama_context_params llama_context_default_params(void);

struct llama_context * llama_init_from_model(
        struct llama_model          * model,
        struct llama_context_params   params);

void llama_free(struct llama_context * ctx);

int32_t llama_n_ctx(const struct llama_context * ctx);

/*
 * ----------------------------------------------------------------------------
 *  Memory (KV cache)
 * ----------------------------------------------------------------------------
 */

struct llama_memory * llama_get_memory(struct llama_context * ctx);

void llama_memory_clear(struct llama_memory * memory, bool clear_all);

/*
 * ----------------------------------------------------------------------------
 *  Tokenization
 * ----------------------------------------------------------------------------
 */

int32_t llama_tokenize(
        const struct llama_vocab * vocab,
        const char               * text,
        int32_t                    text_len,
        llama_token              * tokens,
        int32_t                    n_tokens_max,
        bool                       add_special,
        bool                       parse_special);

llama_token llama_vocab_eos(const struct llama_vocab * vocab);

int32_t llama_token_to_piece(
        const struct llama_vocab * vocab,
        llama_token                token,
        char                     * buf,
        int32_t                    length,
        int32_t                    lstrip,
        bool                       special);

/*
 * ----------------------------------------------------------------------------
 *  Batch helpers
 * ----------------------------------------------------------------------------
 */

struct llama_batch llama_batch_init(
        int32_t n_tokens,
        int32_t embd,
        int32_t n_seq_max);

void llama_batch_free(struct llama_batch batch);

#ifdef __cplusplus
/* C++ convenience wrappers (declared in llama.cpp upstream as inline/extern) */
void llama_batch_add(
        struct llama_batch                  & batch,
        llama_token                           id,
        llama_pos                             pos,
        std::initializer_list<llama_seq_id>   seq_ids,
        bool                                  logits);

void llama_batch_clear(struct llama_batch & batch);
#endif

/*
 * ----------------------------------------------------------------------------
 *  Decode
 * ----------------------------------------------------------------------------
 */

int32_t llama_decode(
        struct llama_context * ctx,
        struct llama_batch     batch);

/*
 * ----------------------------------------------------------------------------
 *  Embeddings
 * ----------------------------------------------------------------------------
 */

const float * llama_get_embeddings_seq(
        struct llama_context * ctx,
        llama_seq_id           seq_id);

/*
 * ----------------------------------------------------------------------------
 *  Sampler
 * ----------------------------------------------------------------------------
 */

struct llama_sampler_chain_params llama_sampler_chain_default_params(void);

struct llama_sampler * llama_sampler_chain_init(
        struct llama_sampler_chain_params params);

void llama_sampler_chain_add(
        struct llama_sampler * chain,
        struct llama_sampler * smpl);

struct llama_sampler * llama_sampler_init_top_k(int32_t k);
struct llama_sampler * llama_sampler_init_top_p(float p, size_t min_keep);
struct llama_sampler * llama_sampler_init_temp (float t);
struct llama_sampler * llama_sampler_init_dist (uint32_t seed);

llama_token llama_sampler_sample(
        struct llama_sampler * smpl,
        struct llama_context * ctx,
        int32_t                idx);

void llama_sampler_free(struct llama_sampler * smpl);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* LLAMA_H */
