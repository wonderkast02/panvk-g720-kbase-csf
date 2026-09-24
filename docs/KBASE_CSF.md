# Kbase / CSF — hardware e bring-up

Este documento preserva detalhes técnicos de baixo nível do caminho G720.

## Plataforma de referência

- SoC: MediaTek MT6899
- GPU: Mali-G720 MC8
- GPU ID: `0xc8700010`
- Vendor ID: `0x13b5`
- Kbase: r49p1 no hardware de referência histórico
- UK version observada: 1.30
- device: `/dev/mali0`
- firmware CSF observado: `mali_csffw.bin`

Esses valores descrevem o dispositivo de referência e não são requisitos universais de toda Mali-G720.

## UAPI / operações validadas

Entre as operações confirmadas historicamente:

- `VERSION_CHECK_CSF`
- `SET_FLAGS`
- `GPUPROPS`
- `CS_GET_GLB_IFACE`
- `MEM_ALLOC_EX` / `BASE_MEM_SAME_VA`
- mmap de BO
- leitura/escrita CPU ↔ BO
- `MEM_QUERY`
- `MEM_COMMIT`
- criação de CSG
- registro, bind e kick de filas CSF
- execução real de comandos na GPU

Incompatibilidade genérica de ioctl deixou de ser a hipótese principal após essa qualificação.

## Correção CSF específica observada na G720

A antiga suposição fixa:

```c
.cs_reg_count = 96,
```

podia terminar em:

```text
overflowed register file
```

A correção passou a derivar a quantidade real informada pela interface CSF:

```c
.cs_reg_count = (stream_features & 0xff) + 1,
```

e a reserva de registradores não preservados foi ajustada para respeitar a interface:

```c
.nr_kernel_registers =
   MAX2(csif_info->unpreserved_cs_reg_count, 4)
```

No dispositivo testado, `stream_features` implicava aproximadamente 114 registradores, eliminando o abort observado naquele caminho.

## Memória / dma-heap

Na lineage histórica e atual, o **device node** do dma-heap pode ser aberto com:

```text
O_RDONLY | O_CLOEXEC
```

Isso não torna os dma-buf FDs alocados read-only: a solicitação de alocação mantém os flags apropriados para os FDs retornados.

## GPU WAIT64 interno

A Beta 2 deriva da autoridade técnica que promoveu espera GPU para dependências binárias locais/internas elegíveis.

Boundary:

- local/internal eligible: CS `SYNC64` wait;
- imported `sync_file`: CPU/KCPU fallback;
- timeline wrapper: fallback;
- mixed waits: fallback;
- wait-only submit: fallback;
- mais de três wait cells: fallback.

A condição validada é `GREATER(target - 1)`, e as esperas são emitidas antes de resource acquisition para evitar bloquear recursos necessários ao producer.

## Modificadores DRM

Regra do projeto:

> Nunca converter `DRM_FORMAT_MOD_INVALID` em `DRM_FORMAT_MOD_LINEAR` por suposição.

Qualquer fallback ou escolha de modifier deve ser sustentado pelo contrato real do caminho de memória/WSI.
