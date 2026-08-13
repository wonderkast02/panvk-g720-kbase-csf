#ifndef LIBKBASE_CSF_H
#define LIBKBASE_CSF_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    int fd;
    uint64_t ring_gpu_va;
    void *ring_cpu;
    uint64_t mmap_handle;
    void *io_base;
    volatile uint32_t *input;
    volatile uint32_t *output;
    uint8_t group_handle;
} kbase_csf_ctx;

int kbase_csf_init(kbase_csf_ctx *ctx);
int kbase_csf_alloc_buffer(kbase_csf_ctx *ctx, size_t size, uint64_t *gpu_va, void **cpu_ptr);
int kbase_csf_create_queue(kbase_csf_ctx *ctx, uint64_t ring_gpu_va, void *ring_cpu);
int kbase_csf_submit(kbase_csf_ctx *ctx, uint64_t *commands, uint32_t num_commands);
int kbase_csf_wait(kbase_csf_ctx *ctx, uint32_t expected_extract);
void kbase_csf_cleanup(kbase_csf_ctx *ctx);

#endif
