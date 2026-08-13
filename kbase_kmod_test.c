#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

#include "libkbase_csf.h"
#include "kbase_kmod_test.h"

static struct kbase_kmod_bo *kbase_bo_alloc(struct kbase_kmod_dev *dev, uint64_t size, uint32_t flags) {
    struct kbase_kmod_bo *bo = calloc(1, sizeof(*bo));
    if (!bo) return NULL;

    if (kbase_csf_alloc_buffer(&dev->ctx, size, &bo->gpu_va, &bo->cpu_ptr) != 0) {
        free(bo);
        return NULL;
    }
    bo->size = size;
    bo->dev = dev;
    return bo;
}

static void kbase_bo_free(struct kbase_kmod_bo *bo) {
    // Liberar memória GPU (não implementado, mas ok para teste)
    free(bo);
}

static void *kbase_bo_map(struct kbase_kmod_bo *bo) {
    return bo->cpu_ptr;
}

static struct kbase_kmod_vm *kbase_vm_create(struct kbase_kmod_dev *dev) {
    struct kbase_kmod_vm *vm = calloc(1, sizeof(*vm));
    vm->dev = dev;
    return vm;
}

static void kbase_vm_destroy(struct kbase_kmod_vm *vm) {
    free(vm);
}

static int kbase_vm_map(struct kbase_kmod_vm *vm, struct kbase_kmod_bo *bo, uint64_t *gpu_va) {
    *gpu_va = bo->gpu_va;
    return 0;
}

struct kbase_kmod_dev *kbase_kmod_dev_create(int fd) {
    struct kbase_kmod_dev *dev = calloc(1, sizeof(*dev));
    if (!dev) return NULL;

    if (kbase_csf_init(&dev->ctx) != 0) {
        free(dev);
        return NULL;
    }

    dev->ops.bo_alloc = kbase_bo_alloc;
    dev->ops.bo_free = kbase_bo_free;
    dev->ops.bo_map = kbase_bo_map;
    dev->ops.vm_create = kbase_vm_create;
    dev->ops.vm_destroy = kbase_vm_destroy;
    dev->ops.vm_map = kbase_vm_map;

    return dev;
}
