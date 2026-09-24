# Arquitetura

## Caminho principal

```text
Aplicação Vulkan
       │
       ▼
   Mesa / PanVK
       │
       ▼
  Backend Kbase
       │
       ▼
     CSF / KMD
       │
       ▼
   Mali-G720 GPU
```

O objetivo arquitetural é executar PanVK diretamente sobre a interface Kbase/CSF disponível no Android.

## Camadas

### Mesa / PanVK

Responsável pela API Vulkan, compilação de shaders, estado gráfico, command paths e integração com as bibliotecas Panfrost/libpan pertinentes.

### Backend Kbase

Faz a ponte userspace com `/dev/mali0` e a UAPI Kbase necessária para memória, propriedades, filas, sincronização e submissão.

### CSF / KMD

A infraestrutura Command Stream Frontend e o kernel driver executam a comunicação de baixo nível com a GPU.

## Sincronização interna

Na autoridade técnica atual, dependências binárias locais/internas PanVK elegíveis podem permanecer na GPU:

```text
producer → local binary payload → CS SYNC64/WAIT64 → consumer
```

Contrato qualificado:

- `GREATER(target - 1)`;
- no máximo três wait cells por submit path;
- waits emitidos antes da aquisição de recursos.

Não é substituição da sincronização externa. Continuam no fallback CPU/KCPU:

- imported `sync_file`;
- timeline wrappers;
- mixed wait sets;
- wait-only submits;
- oversized wait sets.

## Compatibilidade externa

Winlator, Vortek, Wine, Box64, DXVK e VKD3D são camadas externas ao driver.

Elas podem:

- adicionar requisitos Vulkan;
- alterar transporte;
- introduzir wrappers;
- afetar sincronização;
- transformar o ambiente de execução.

Uma falha nessas camadas não é automaticamente uma falha PanVK.

## Princípios

- não virtualizar feature por simples advertisement;
- separar emulação/wrapper de suporte nativo;
- manter Kbase/CSF-specific behavior em camada apropriada;
- validar runtime no hardware;
- correctness antes de performance;
- tratar compatibilidade de jogo como camada posterior à correção do driver.

## Hardware de referência

A validação principal atual usa MediaTek MT6899 + Mali-G720 MC8. Isso não implica que toda G720 possua exatamente a mesma combinação de kernel, firmware, UAPI ou comportamento.
