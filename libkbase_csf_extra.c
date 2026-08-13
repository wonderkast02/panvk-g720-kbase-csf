#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

#define IS_ENABLED(x) 0
#define MALI_USE_CSF 1

#include "mali_kbase_ioctl.h"
#include "mali_base_kernel.h"
#include "libkbase_csf_extra.h"

uint64_t kbase_csf_alloc_buffer_cookie(kbase_csf_ctx *ctx, size_t size) {
    union kbase_ioctl_mem_alloc alloc;
    memset(&alloc, 0, sizeof(alloc));
    alloc.in.va_pages = (size + 4095) / 4096;
    alloc.in.commit_pages = alloc.in.va_pages;
    alloc.in.extension = 0;
    alloc.in.flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR |
                     BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR |
                     BASE_MEM_SAME_VA;
    if (ioctl(ctx->fd, KBASE_IOCTL_MEM_ALLOC, &alloc)) {
        perror("MEM_ALLOC (cookie)");
        return 0;
    }
    return alloc.out.gpu_va;
}

void *kbase_csf_map_buffer(kbase_csf_ctx *ctx, uint64_t cookie, size_t size) {
    void *ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, ctx->fd, (off_t)cookie);
    if (ptr == MAP_FAILED) {
        perror("mmap buffer");
        return NULL;
    }
    return ptr;
}
