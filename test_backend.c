#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "kbase_kmod_test.h"

int main() {
    int fd = open("/dev/mali0", O_RDWR);
    if (fd < 0) return 1;

    struct kbase_kmod_dev *dev = kbase_kmod_dev_create(fd);
    struct kbase_kmod_vm *vm = dev->ops.vm_create(dev);

    struct kbase_kmod_bo *bo = dev->ops.bo_alloc(dev, 4096, 0);
    if (!bo) { printf("bo_alloc falhou\n"); return 1; }

    uint64_t gpu_va;
    if (dev->ops.vm_map(vm, bo, &gpu_va) != 0) { printf("vm_map falhou\n"); return 1; }

    void *cpu_ptr = dev->ops.bo_map(bo);
    *(uint32_t*)cpu_ptr = 0xDEADBEEF;

    printf("Valor escrito: 0x%x\n", *(volatile uint32_t*)cpu_ptr);

    dev->ops.bo_free(bo);
    dev->ops.vm_destroy(vm);
    free(dev);
    close(fd);
    return 0;
}
