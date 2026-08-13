#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "libkbase_csf.h"

#define OP_MOVE48          1
#define OP_LOAD_MULTIPLE   20
#define OP_STORE_MULTIPLE  21
#define OP_WAIT            3
#define OP_NOP             0

static uint64_t make_move48(uint64_t imm48, uint8_t dest) {
    return (imm48 & ((1ULL<<48)-1)) | ((uint64_t)dest << 48) | ((uint64_t)OP_MOVE48 << 56);
}

static uint64_t make_load_multiple(uint8_t addr_reg, uint16_t offset, uint16_t mask, uint8_t base_reg) {
    return ((uint64_t)offset & 0xffff) |
           ((uint64_t)mask << 16) |
           ((uint64_t)addr_reg << 40) |
           ((uint64_t)base_reg << 48) |
           ((uint64_t)OP_LOAD_MULTIPLE << 56);
}

static uint64_t make_store_multiple(uint8_t addr_reg, uint16_t offset, uint16_t mask, uint8_t base_reg) {
    return ((uint64_t)offset & 0xffff) |
           ((uint64_t)mask << 16) |
           ((uint64_t)addr_reg << 40) |
           ((uint64_t)base_reg << 48) |
           ((uint64_t)OP_STORE_MULTIPLE << 56);
}

static uint64_t make_wait(uint16_t mask) {
    return ((uint64_t)mask << 16) |
           ((uint64_t)OP_WAIT << 56);
}

int main() {
    kbase_csf_ctx ctx = {0};
    if (kbase_csf_init(&ctx) != 0) { printf("init falhou\n"); return 1; }

    uint64_t src_gpu, dst_gpu;
    void *src_cpu, *dst_cpu;
    if (kbase_csf_alloc_buffer(&ctx, 4096, &src_gpu, &src_cpu) != 0) return 1;
    if (kbase_csf_alloc_buffer(&ctx, 4096, &dst_gpu, &dst_cpu) != 0) return 1;

    *(uint32_t*)src_cpu = 0x12345678;
    *(uint32_t*)dst_cpu = 0;

    __builtin___clear_cache((char*)src_cpu, (char*)src_cpu + 4096);
    __builtin___clear_cache((char*)dst_cpu, (char*)dst_cpu + 4096);

    printf("Origem: 0x%x, Destino inicial: 0x%x\n", *(uint32_t*)src_cpu, *(uint32_t*)dst_cpu);

    uint64_t ring_gpu;
    void *ring_cpu;
    if (kbase_csf_alloc_buffer(&ctx, 4096, &ring_gpu, &ring_cpu) != 0) return 1;
    if (kbase_csf_create_queue(&ctx, ring_gpu, ring_cpu) != 0) return 1;

    uint64_t cmds[6];
    cmds[0] = make_move48(src_gpu, 0);                 // r0/r1 = src
    cmds[1] = make_load_multiple(0, 0, 0x1, 4);        // r4 = *(r0+0)
    cmds[2] = make_wait(0x1);                          // WAIT for load scoreboard
    cmds[3] = make_move48(dst_gpu, 2);                 // r2/r3 = dst
    cmds[4] = make_store_multiple(2, 0, 0x1, 4);       // *(r2+0) = r4
    cmds[5] = OP_NOP;

    if (kbase_csf_submit(&ctx, cmds, 6) != 0) return 1;
    if (kbase_csf_wait(&ctx, 6*8) != 0) { printf("timeout\n"); return 1; }

    __builtin___clear_cache((char*)dst_cpu, (char*)dst_cpu + 4096);

    uint32_t valor = *(volatile uint32_t*)dst_cpu;
    printf("Valor copiado para o destino: 0x%x\n", valor);
    if (valor == 0x12345678) {
        printf(">>> SUCESSO: GPU copiou memória! <<<\n");
    } else {
        printf(">>> FALHA: valor destino incorreto. <<<\n");
    }

    kbase_csf_cleanup(&ctx);
    return 0;
}
