# Histórico técnico

Este documento preserva os principais marcos que antes ocupavam o README. Ele não substitui `PROVENANCE.md` nem `VALIDATION.md`.

## 1. Kbase / CSF bring-up

A primeira fase estabeleceu a comunicação funcional com a Mali-G720 através de `/dev/mali0`.

Foram trabalhados e validados:

- handshake/UAPI;
- propriedades da GPU;
- memória e BOs;
- mmap;
- leitura/escrita CPU ↔ BO;
- `MEM_QUERY` / `MEM_COMMIT`;
- CSG;
- filas CSF;
- execução real de comandos na GPU.

A correção histórica do register file CSF está documentada em `KBASE_CSF.md`.

## 2. PanVK Vulkan nativo

O projeto avançou para:

```text
Vulkan
  ↓
Mesa / PanVK
  ↓
Kbase / CSF
  ↓
Mali-G720
```

Marcos dirigidos incluíram:

- `vulkaninfo`;
- enumeração da Mali-G720;
- compute;
- graphics pipeline;
- triângulo offscreen;
- readback para CPU;
- texture sampling;
- depth D32;
- alpha blending;
- stencil D24S8;
- MSAA 4x + resolve;
- stress/repetição.

## 3. WSI / Termux:X11

Foram preservados testes históricos de:

- swapchain Vulkan X11 ARM64;
- `vkcube` via Termux:X11;
- execução prolongada com centenas de frames.

Esses testes comprovaram o caminho WSI específico, não conformidade Vulkan geral.

## 4. Box64 / Wine

O projeto validou a passagem de workloads x86_64/Windows por Box64 e Wine até o loader Vulkan AArch64 e o PanVK.

Detalhes ficam em `COMPATIBILITY.md`.

## 5. DXVK G720 LAB

Uma frente experimental usou um bypass de gate para investigar DXVK além da rejeição inicial de features.

O objetivo era diagnóstico, não fingir implementação de features ausentes.

Foram preservados resultados de criação D3D11 experimental, draw/readback e PresentImmediate.

## 6. Investigação de BCn

A leitura de texture feature bits no hardware de referência não anunciou BC1–BC7 no caminho investigado.

Isso levou à regra de não anunciar `textureCompressionBC` sem implementação real, mesmo quando wrappers/drivers proprietários aparentavam oferecer a feature.

## 7. Wrapper Bionic → glibc

O `bionic-vulkan-wrapper` foi portado experimentalmente para investigar virtualização/emulação de features e comparar o caminho indireto com o PanVK nativo.

O snapshot histórico chegou próximo ao link final, mas continuava dependente de passes customizados do SPIRV-Tools naquele checkpoint.

Com o amadurecimento do PanVK nativo, o wrapper deixou de ser o caminho principal.

## 8. Transição definitiva para PanVK nativo

A arquitetura de desenvolvimento foi consolidada em:

```text
Aplicação Vulkan
       ↓
Mesa / PanVK
       ↓
Kbase backend
       ↓
Kbase / CSF
       ↓
Mali-G720
```

Wrappers passaram a ser ferramentas de pesquisa/compatibilidade, não parte do núcleo do driver.

## 9. Tessellation compiler bring-up

O trabalho de tessellation incluiu os estágios:

- VS → COMPUTE;
- TCS → COMPUTE;
- TES → VERTEX;
- metadata;
- binding/state;
- descriptors;
- poly sysvals.

## 10. Tessellation precompiled kernels

Entre os kernels preservados na investigação estão:

- `panlib_prefix_sum_tess`
- `panlib_tess_isoline`
- `panlib_tess_tri`
- `panlib_tess_quad`

## 11. Tessellation direct-runtime

A validação em hardware trabalhou, entre outros pontos:

- software VS → TCS;
- libpoly COUNT → prefix sum → WITH_COUNTS;
- geração de indexed indirect draw;
- TES/IDVS → rasterização;
- readback semântico;
- `gl_TessCoord`;
- TES → FS user varying;
- triangles / quads / isolines;
- equal spacing;
- fractional-even / fractional-odd;
- common-edge;
- winding;
- stress;
- CTS focado.

O checkpoint histórico `0521a3257628` permanece parte da proveniência.

Os resultados quantitativos atuais preservados estão em `VALIDATION.md`.

## 12. Geometry Shader

Após tessellation, a linha de desenvolvimento avançou para Geometry Shader.

A qualificação foi conduzida em boundaries causais, evitando reabrir testes já fechados sem regressão real.

O closeout dirigido pós-beta cobriu:

- semântica GS representativa;
- draws diretos/indiretos relevantes;
- limites representativos;
- Tessellation + GS;
- Transform Feedback + GS;
- Layered GS;
- regressão integrada.

Tooling, transporte e instrumentação foram tratados separadamente de falhas reais do driver.

## 13. Estado pós-GS

Com o fechamento funcional de GS, a prioridade passou a ser:

- reconstrução/consolidação limpa da source tree;
- preservação de autoridade e hashes;
- reprodutibilidade;
- Winlator/Vortek;
- DXVK;
- regressões;
- performance somente depois da correção.

A release pública continua `0.1.0-beta.1.9.4` até decisão explícita de nova publicação.
