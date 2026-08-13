#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <xf86drm.h>

#include "libkbase_csf.h"
#include "pan_kmod.h"

/* Estrutura privada do dispositivo */
struct kbase_pan_kmod_dev {
    struct pan_kmod_dev base;
    kbase_csf_ctx csf_ctx;    // contexto CSF já inicializado
};

/* Estrutura privada do BO */
struct kbase_pan_kmod_bo {
    struct pan_kmod_bo base;
    uint64_t gpu_va;          // endereço real da GPU (SAME_VA)
    void *cpu_ptr;            // mapeamento CPU
};

/* Estrutura privada da VM */
struct kbase_pan_kmod_vm {
    struct pan_kmod_vm base;
};

/* Funções auxiliares */
static struct kbase_pan_kmod_dev *
to_kbase_dev(struct pan_kmod_dev *dev) {
    return container_of(dev, struct kbase_pan_kmod_dev, base);
}

static struct kbase_pan_kmod_bo *
to_kbase_bo(struct pan_kmod_bo *bo) {
    return container_of(bo, struct kbase_pan_kmod_bo, base);
}

/* ---------- Implementação das operações ---------- */

static struct pan_kmod_dev *
kbase_kmod_dev_create(int fd, uint32_t flags,
                      const struct pan_kmod_driver *drv_info,
                      const struct pan_kmod_allocator *allocator)
{
    struct kbase_pan_kmod_dev *kdev = pan_kmod_alloc(allocator, sizeof(*kdev));
    if (!kdev)
        return NULL;

    /* Inicializa contexto Kbase CSF */
    if (kbase_csf_init(&kdev->csf_ctx) != 0) {
        pan_kmod_free(allocator, kdev);
        return NULL;
    }

    /* Inicializa a base */
    pan_kmod_dev_init(&kdev->base, fd, flags, drv_info, &kbase_kmod_ops, allocator);

    /* Preenche propriedades mínimas (pode ser expandido) */
    kdev->base.props.gpu_id = 0xc870; // Mali-G720
    kdev->base.props.shader_present = 0xff; // 8 núcleos
    kdev->base.props.mmu_features = 48; // 48 bits VA
    kdev->base.props.pgsize_bitmap = PAN_PGSIZE_4K;

    return &kdev->base;
}

static void
kbase_kmod_dev_destroy(struct pan_kmod_dev *dev)
{
    struct kbase_pan_kmod_dev *kdev = to_kbase_dev(dev);
    kbase_csf_cleanup(&kdev->csf_ctx);
    pan_kmod_dev_cleanup(dev);
    pan_kmod_free(dev->allocator, kdev);
}

static struct pan_kmod_bo *
kbase_kmod_bo_alloc(struct pan_kmod_dev *dev,
                    struct pan_kmod_vm *exclusive_vm,
                    uint64_t size, uint32_t flags)
{
    struct kbase_pan_kmod_dev *kdev = to_kbase_dev(dev);
    struct kbase_pan_kmod_bo *kbo = pan_kmod_dev_alloc(dev, sizeof(*kbo));
    if (!kbo)
        return NULL;

    /* Ignorando flags específicos por enquanto; usamos SAME_VA */
    uint64_t gpu_va;
    void *cpu_ptr;
    if (kbase_csf_alloc_buffer(&kdev->csf_ctx, size, &gpu_va, &cpu_ptr) != 0) {
        pan_kmod_dev_free(dev, kbo);
        return NULL;
    }

    kbo->gpu_va = gpu_va;
    kbo->cpu_ptr = cpu_ptr;
    pan_kmod_bo_init(&kbo->base, dev, exclusive_vm, size, flags, 0);

    return &kbo->base;
}

static void
kbase_kmod_bo_free(struct pan_kmod_bo *bo)
{
    /* Não implementamos MEM_FREE; vazamento controlado por enquanto */
    struct kbase_pan_kmod_bo *kbo = to_kbase_bo(bo);
    pan_kmod_dev_free(bo->dev, kbo);
}

static off_t
kbase_kmod_bo_get_mmap_offset(struct pan_kmod_bo *bo)
{
    /* Retorna um cookie falso; a mmap real será tratada pelo bo_map? */
    return 0;
}

static void *
kbase_kmod_bo_mmap(struct pan_kmod_bo *bo, int prot, int flags, void *host_addr)
{
    struct kbase_pan_kmod_bo *kbo = to_kbase_bo(bo);
    /* Como já temos o mapeamento, retornamos o ponteiro diretamente */
    return kbo->cpu_ptr;
}

static struct pan_kmod_vm *
kbase_kmod_vm_create(struct pan_kmod_dev *dev, uint32_t flags,
                     uint64_t va_start, uint64_t va_range)
{
    struct kbase_pan_kmod_vm *kvm = pan_kmod_dev_alloc(dev, sizeof(*kvm));
    if (!kvm)
        return NULL;
    pan_kmod_vm_init(&kvm->base, dev, 0, flags);
    return &kvm->base;
}

static void
kbase_kmod_vm_destroy(struct pan_kmod_vm *vm)
{
    struct kbase_pan_kmod_vm *kvm = container_of(vm, struct kbase_pan_kmod_vm, base);
    pan_kmod_vm_cleanup(vm);
    pan_kmod_dev_free(vm->dev, kvm);
}

static int
kbase_kmod_vm_bind(struct pan_kmod_vm *vm, enum pan_kmod_vm_op_mode mode,
                   struct pan_kmod_vm_op *ops, uint32_t op_count)
{
    /* No Kbase, o mapeamento já é implícito via SAME_VA; nada a fazer */
    return 0;
}

/* Tabela de operações */
const struct pan_kmod_ops kbase_kmod_ops = {
    .dev_create = kbase_kmod_dev_create,
    .dev_destroy = kbase_kmod_dev_destroy,
    .bo_alloc = kbase_kmod_bo_alloc,
    .bo_free = kbase_kmod_bo_free,
    .bo_get_mmap_offset = kbase_kmod_bo_get_mmap_offset,
    .bo_wait = NULL,
    .vm_create = kbase_kmod_vm_create,
    .vm_destroy = kbase_kmod_vm_destroy,
    .vm_bind = kbase_kmod_vm_bind,
};
