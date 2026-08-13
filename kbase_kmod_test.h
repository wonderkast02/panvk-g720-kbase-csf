#ifndef KBASE_KMOD_TEST_H
#define KBASE_KMOD_TEST_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "libkbase_csf.h"

struct kbase_kmod_dev;
struct kbase_kmod_bo;
struct kbase_kmod_vm;

struct kbase_kmod_ops {
    struct kbase_kmod_bo *(*bo_alloc)(struct kbase_kmod_dev *dev, uint64_t size, uint32_t flags);
    void (*bo_free)(struct kbase_kmod_bo *bo);
    void *(*bo_map)(struct kbase_kmod_bo *bo);
    struct kbase_kmod_vm *(*vm_create)(struct kbase_kmod_dev *dev);
    void (*vm_destroy)(struct kbase_kmod_vm *vm);
    int (*vm_map)(struct kbase_kmod_vm *vm, struct kbase_kmod_bo *bo, uint64_t *gpu_va);
};

struct kbase_kmod_dev {
    kbase_csf_ctx ctx;
    struct kbase_kmod_ops ops;
};

struct kbase_kmod_bo {
    uint64_t gpu_va;
    void *cpu_ptr;
    uint64_t size;
    struct kbase_kmod_dev *dev;
};

struct kbase_kmod_vm {
    struct kbase_kmod_dev *dev;
};

struct kbase_kmod_dev *kbase_kmod_dev_create(int fd);

#endif
