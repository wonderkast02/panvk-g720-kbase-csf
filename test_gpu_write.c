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

#define CS_INSERT_LO 0x0000
#define CS_INSERT_HI 0x0004
#define CS_EXTRACT_INIT_LO 0x0008
#define CS_EXTRACT_INIT_HI 0x000C
#define CS_EXTRACT_LO 0x0000
#define CS_ACTIVE 0x0008

#define OP_MOVE48      1
#define OP_MOVE32      2
#define OP_SYNC_SET32 38
#define OP_NOP         0

static inline void mfence(void) { __asm__ volatile("dmb ish" ::: "memory"); }

static uint64_t make_move48(uint64_t imm48, uint8_t dest) {
    return (imm48 & ((1ULL<<48)-1)) | ((uint64_t)dest << 48) | ((uint64_t)OP_MOVE48 << 56);
}

static uint64_t make_move32(uint32_t imm32, uint8_t dest) {
    return (uint64_t)imm32 | ((uint64_t)dest << 48) | ((uint64_t)OP_MOVE32 << 56);
}

static uint64_t make_sync_set32(uint8_t data_reg, uint8_t addr_reg) {
    // fields: error(0), scope(0), wait_mask(0), data(reg), addr(reg), signal_slot(0), defer_mode(0), opcode
    return ((uint64_t)OP_SYNC_SET32 << 56) |
           ((uint64_t)data_reg << 32) |
           ((uint64_t)addr_reg << 40);
}

int main() {
    int fd = open("/dev/mali0", O_RDWR);
    if (fd < 0) { perror("open"); return 1; }

    // --- 1. Inicialização do contexto ---
    struct kbase_ioctl_version_check version = {0};
    version.major = 1; version.minor = 30;
    if (ioctl(fd, KBASE_IOCTL_VERSION_CHECK, &version)) {
        printf("VERSION_CHECK failed: %s\n", strerror(errno)); close(fd); return 1;
    }
    struct kbase_ioctl_set_flags set_flags = {0};
    set_flags.create_flags = 0;
    if (ioctl(fd, KBASE_IOCTL_SET_FLAGS, &set_flags)) {
        printf("SET_FLAGS failed: %s\n", strerror(errno)); close(fd); return 1;
    }
    printf("Contexto criado.\n");

    // --- 2. Alocar buffer de dados (onde a GPU vai escrever) ---
    union kbase_ioctl_mem_alloc data_alloc;
    memset(&data_alloc, 0, sizeof(data_alloc));
    data_alloc.in.va_pages = 1;
    data_alloc.in.commit_pages = 1;
    data_alloc.in.extension = 0;
    data_alloc.in.flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR |
                          BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR |
                          BASE_MEM_SAME_VA;
    if (ioctl(fd, KBASE_IOCTL_MEM_ALLOC, &data_alloc)) {
        printf("MEM_ALLOC (data) failed: %s\n", strerror(errno)); close(fd); return 1;
    }
    uint64_t data_cookie = data_alloc.out.gpu_va;
    void *data_cpu = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)data_cookie);
    if (data_cpu == MAP_FAILED) { printf("mmap data failed\n"); close(fd); return 1; }
    uint64_t data_gpu_va = (uint64_t)data_cpu;
    printf("Buffer de dados GPU VA: 0x%llx, CPU: %p\n", (unsigned long long)data_gpu_va, data_cpu);
    // Limpar o buffer
    *(uint32_t*)data_cpu = 0;

    // --- 3. Alocar ring buffer (1 página) ---
    union kbase_ioctl_mem_alloc ring_alloc;
    memset(&ring_alloc, 0, sizeof(ring_alloc));
    ring_alloc.in.va_pages = 1;
    ring_alloc.in.commit_pages = 1;
    ring_alloc.in.extension = 0;
    ring_alloc.in.flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR |
                          BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR |
                          BASE_MEM_SAME_VA;
    if (ioctl(fd, KBASE_IOCTL_MEM_ALLOC, &ring_alloc)) {
        printf("MEM_ALLOC (ring) failed: %s\n", strerror(errno)); close(fd); return 1;
    }
    uint64_t ring_cookie = ring_alloc.out.gpu_va;
    void *ring_cpu = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)ring_cookie);
    if (ring_cpu == MAP_FAILED) { printf("mmap ring failed\n"); close(fd); return 1; }
    uint64_t ring_gpu_va = (uint64_t)ring_cpu;
    printf("Ring buffer GPU VA: 0x%llx\n", (unsigned long long)ring_gpu_va);

    // --- 4. Criar CSG ---
    union kbase_ioctl_cs_queue_group_create csg;
    memset(&csg, 0, sizeof(csg));
    csg.in.tiler_mask = 1;
    csg.in.fragment_mask = 0xFF;
    csg.in.compute_mask = 0xFF;
    csg.in.cs_min = 1; csg.in.priority = 0;
    csg.in.tiler_max = 1; csg.in.fragment_max = 8; csg.in.compute_max = 8;
    csg.in.csi_handlers = 0; csg.in.reserved = 0; csg.in.cs_fault_report_enable = 0; csg.in.dvs_buf = 0;
    if (ioctl(fd, KBASE_IOCTL_CS_QUEUE_GROUP_CREATE, &csg)) {
        printf("CSG_CREATE failed: %s\n", strerror(errno)); close(fd); return 1;
    }
    uint8_t group_handle = csg.out.group_handle;

    // --- 5. Registrar fila ---
    struct kbase_ioctl_cs_queue_register reg;
    memset(&reg, 0, sizeof(reg));
    reg.buffer_gpu_addr = ring_gpu_va; reg.buffer_size = 4096; reg.priority = 0;
    if (ioctl(fd, KBASE_IOCTL_CS_QUEUE_REGISTER, &reg)) {
        printf("QUEUE_REGISTER failed: %s\n", strerror(errno)); close(fd); return 1;
    }

    // --- 6. Bind ---
    union kbase_ioctl_cs_queue_bind bind;
    memset(&bind, 0, sizeof(bind));
    bind.in.buffer_gpu_addr = ring_gpu_va; bind.in.group_handle = group_handle; bind.in.csi_index = 0;
    if (ioctl(fd, KBASE_IOCTL_CS_QUEUE_BIND, &bind)) {
        printf("QUEUE_BIND failed: %s\n", strerror(errno)); close(fd); return 1;
    }
    uint64_t mmap_handle = bind.out.mmap_handle;

    // --- 7. Mapear páginas de interface (doorbell, input, output) ---
    void *io_base = mmap(NULL, 3*4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)mmap_handle);
    if (io_base == MAP_FAILED) { printf("mmap interface failed\n"); close(fd); return 1; }

    volatile uint32_t *input = (volatile uint32_t *)((uint8_t *)io_base + 4096);
    volatile uint32_t *output = (volatile uint32_t *)((uint8_t *)io_base + 8192);

    // --- 8. Inicializar registradores da página de input ---
    input[CS_INSERT_LO/4] = 0;
    input[CS_INSERT_HI/4] = 0;
    input[CS_EXTRACT_INIT_LO/4] = 0;
    input[CS_EXTRACT_INIT_HI/4] = 0;
    mfence();

    // --- 9. Escrever comandos no ring buffer ---
    uint64_t *ring = (uint64_t *)ring_cpu;
    ring[0] = make_move48(data_gpu_va, 0);          // carrega endereço do buffer nos registradores r0/r1
    ring[1] = make_move32(0xDEADBEEF, 2);           // carrega valor no registrador r2
    ring[2] = make_sync_set32(2, 0);                // escreve r2 no endereço apontado por r0
    ring[3] = OP_NOP;                               // NOP final
    mfence();

    // --- 10. Atualizar insert pointer para 4 instruções (32 bytes) ---
    input[CS_INSERT_LO/4] = 4 * 8;  // 32 bytes
    input[CS_INSERT_HI/4] = 0;
    mfence();

    printf("Insert após write: 0x%x\n", input[CS_INSERT_LO/4]);

    // --- 11. Kick ---
    struct kbase_ioctl_cs_queue_kick kick;
    memset(&kick, 0, sizeof(kick));
    kick.buffer_gpu_addr = ring_gpu_va;
    if (ioctl(fd, KBASE_IOCTL_CS_QUEUE_KICK, &kick)) {
        printf("QUEUE_KICK failed: %s\n", strerror(errno)); close(fd); return 1;
    }
    printf("Kick enviado.\n");

    // --- 12. Aguardar execução (monitorar extract) ---
    for (int i = 0; i < 20; i++) {
        usleep(100000);
        uint32_t extract = output[CS_EXTRACT_LO/4];
        printf("t=%dms: extract=%u\n", (i+1)*100, extract);
        if (extract == 32) break;
    }

    // --- 13. Verificar se o valor foi escrito no buffer ---
    uint32_t valor_lido = *(volatile uint32_t *)data_cpu;
    printf("Valor no buffer de dados: 0x%x\n", valor_lido);
    if (valor_lido == 0xDEADBEEF) {
        printf(">>> SUCESSO: GPU escreveu na memória! <<<\n");
    } else {
        printf(">>> FALHA: valor não foi escrito (possível problema de cache ou comando). <<<\n");
    }

    munmap(io_base, 3*4096);
    close(fd);
    return 0;
}
