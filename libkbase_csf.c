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
#include "libkbase_csf.h"

#define CS_INSERT_LO 0x0000
#define CS_INSERT_HI 0x0004
#define CS_EXTRACT_INIT_LO 0x0008
#define CS_EXTRACT_INIT_HI 0x000C
#define CS_EXTRACT_LO 0x0000

static inline void mfence(void) { __asm__ volatile("dmb ish" ::: "memory"); }

int kbase_csf_init(kbase_csf_ctx *ctx) {
    ctx->fd = open("/dev/mali0", O_RDWR);
    if (ctx->fd < 0) return -1;

    struct kbase_ioctl_version_check version = {0};
    version.major = 1; version.minor = 30;
    if (ioctl(ctx->fd, KBASE_IOCTL_VERSION_CHECK, &version)) {
        perror("VERSION_CHECK");
        close(ctx->fd);
        return -1;
    }

    struct kbase_ioctl_set_flags set_flags = {0};
    set_flags.create_flags = 0;
    if (ioctl(ctx->fd, KBASE_IOCTL_SET_FLAGS, &set_flags)) {
        perror("SET_FLAGS");
        close(ctx->fd);
        return -1;
    }
    return 0;
}

int kbase_csf_alloc_buffer(kbase_csf_ctx *ctx, size_t size, uint64_t *gpu_va, void **cpu_ptr) {
    union kbase_ioctl_mem_alloc alloc;
    memset(&alloc, 0, sizeof(alloc));
    alloc.in.va_pages = (size + 4095) / 4096;
    alloc.in.commit_pages = alloc.in.va_pages;
    alloc.in.extension = 0;
    alloc.in.flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR |
                     BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR |
                     BASE_MEM_SAME_VA;
    if (ioctl(ctx->fd, KBASE_IOCTL_MEM_ALLOC, &alloc)) {
        perror("MEM_ALLOC");
        return -1;
    }
    uint64_t cookie = alloc.out.gpu_va;
    void *ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, ctx->fd, (off_t)cookie);
    if (ptr == MAP_FAILED) {
        perror("mmap buffer");
        return -1;
    }
    *gpu_va = (uint64_t)ptr;
    *cpu_ptr = ptr;
    return 0;
}

int kbase_csf_create_queue(kbase_csf_ctx *ctx, uint64_t ring_gpu_va, void *ring_cpu) {
    // Criar CSG
    union kbase_ioctl_cs_queue_group_create csg;
    memset(&csg, 0, sizeof(csg));
    csg.in.tiler_mask = 1;
    csg.in.fragment_mask = 0xFF;
    csg.in.compute_mask = 0xFF;
    csg.in.cs_min = 1; csg.in.priority = 0;
    csg.in.tiler_max = 1; csg.in.fragment_max = 8; csg.in.compute_max = 8;
    csg.in.csi_handlers = 0; csg.in.reserved = 0; csg.in.cs_fault_report_enable = 0; csg.in.dvs_buf = 0;
    if (ioctl(ctx->fd, KBASE_IOCTL_CS_QUEUE_GROUP_CREATE_1_6, &csg)) {
        perror("CSG_CREATE");
        return -1;
    }
    ctx->group_handle = csg.out.group_handle;

    // Registrar fila
    struct kbase_ioctl_cs_queue_register reg;
    memset(&reg, 0, sizeof(reg));
    reg.buffer_gpu_addr = ring_gpu_va; reg.buffer_size = 4096; reg.priority = 1;
    if (ioctl(ctx->fd, KBASE_IOCTL_CS_QUEUE_REGISTER, &reg)) {
        perror("QUEUE_REGISTER");
        return -1;
    }

    // Bind
    union kbase_ioctl_cs_queue_bind bind;
    memset(&bind, 0, sizeof(bind));
    bind.in.buffer_gpu_addr = ring_gpu_va; bind.in.group_handle = ctx->group_handle; bind.in.csi_index = 0;
    if (ioctl(ctx->fd, KBASE_IOCTL_CS_QUEUE_BIND, &bind)) {
        perror("QUEUE_BIND");
        return -1;
    }
    ctx->mmap_handle = bind.out.mmap_handle;

    // Mapear páginas de interface
    ctx->io_base = mmap(NULL, 3*4096, PROT_READ|PROT_WRITE, MAP_SHARED, ctx->fd, (off_t)ctx->mmap_handle);
    if (ctx->io_base == MAP_FAILED) {
        perror("mmap interface");
        return -1;
    }
    ctx->input = (volatile uint32_t *)((uint8_t *)ctx->io_base + 4096);
    ctx->output = (volatile uint32_t *)((uint8_t *)ctx->io_base + 8192);

    // Inicializar registradores
    ctx->input[CS_INSERT_LO/4] = 0;
    ctx->input[CS_INSERT_HI/4] = 0;
    ctx->input[CS_EXTRACT_INIT_LO/4] = 0;
    ctx->input[CS_EXTRACT_INIT_HI/4] = 0;
    mfence();

    ctx->ring_gpu_va = ring_gpu_va;
    ctx->ring_cpu = ring_cpu;
    return 0;
}

int kbase_csf_submit(kbase_csf_ctx *ctx, uint64_t *commands, uint32_t num_commands) {
    uint64_t *ring = (uint64_t *)ctx->ring_cpu;
    for (uint32_t i = 0; i < num_commands; i++) ring[i] = commands[i];
    mfence();

    uint32_t bytes = num_commands * 8;
    ctx->input[CS_INSERT_LO/4] = bytes;
    ctx->input[CS_INSERT_HI/4] = 0;
    mfence();

    struct kbase_ioctl_cs_queue_kick kick;
    memset(&kick, 0, sizeof(kick));
    kick.buffer_gpu_addr = ctx->ring_gpu_va;
    if (ioctl(ctx->fd, KBASE_IOCTL_CS_QUEUE_KICK, &kick)) {
        perror("QUEUE_KICK");
        return -1;
    }
    // Escrever 1 no doorbell (página 0 do user_io)
    if (ctx->io_base) {
        *(volatile uint32_t *)ctx->io_base = 1;
        __sync_synchronize();
    }
    return 0;
}

int kbase_csf_wait(kbase_csf_ctx *ctx, uint32_t expected_extract) {
    for (int i = 0; i < 20; i++) {
        usleep(100000);
        uint32_t extract = ctx->output[CS_EXTRACT_LO/4];
        if (extract >= expected_extract) return 0;
    }
    return -1;
}

void kbase_csf_cleanup(kbase_csf_ctx *ctx) {
    if (ctx->io_base) munmap(ctx->io_base, 3*4096);
    close(ctx->fd);
}
