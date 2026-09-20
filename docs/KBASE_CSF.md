# Kbase / CSF — hardware e bring-up

Este documento preserva os detalhes técnicos de baixo nível retirados do README principal.

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

A UAPI Kbase usada pelo Mesa foi comparada com o driver vendor e os caminhos relevantes do bring-up foram validados.

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

Um problema histórico importante estava na suposição de quantidade fixa de work registers:

```c
.cs_reg_count = 96,
```

No hardware de referência isso podia terminar em:

```text
overflowed register file
```

A correção passou a derivar a quantidade real informada pela interface CSF:

```c
.cs_reg_count = (stream_features & 0xff) + 1,
```

e a reserva de registradores não preservados foi ajustada para respeitar a informação fornecida pela interface:

```c
.nr_kernel_registers =
   MAX2(csif_info->unpreserved_cs_reg_count, 4)
```

No dispositivo testado, `stream_features` implicava aproximadamente 114 registradores, e a correção eliminou o abort observado naquele caminho.

## Memória / dma-heap

Na lineage documentada da beta pública, o **device node** do dma-heap é aberto com:

```text
O_RDONLY | O_CLOEXEC
```

Isso não significa que os dma-buf FDs retornados pela operação de alocação sejam read-only: a solicitação de alocação continua usando os flags documentados para os FDs retornados.

## Modificadores DRM

Regra do projeto:

> Nunca converter `DRM_FORMAT_MOD_INVALID` em `DRM_FORMAT_MOD_LINEAR` por suposição.

Qualquer fallback ou escolha de modifier deve ser sustentado pelo contrato real do caminho de memória/WSI.
