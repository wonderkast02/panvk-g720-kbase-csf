#ifndef LIBKBASE_CSF_EXTRA_H
#define LIBKBASE_CSF_EXTRA_H

#include <stdint.h>
#include <stddef.h>
#include "libkbase_csf.h"

/* Aloca um buffer GPU e retorna o cookie (offset para mmap) */
uint64_t kbase_csf_alloc_buffer_cookie(kbase_csf_ctx *ctx, size_t size);

/* Mapeia um cookie em um ponteiro CPU e retorna o GPU VA (endereço real) */
void *kbase_csf_map_buffer(kbase_csf_ctx *ctx, uint64_t cookie, size_t size);

#endif
