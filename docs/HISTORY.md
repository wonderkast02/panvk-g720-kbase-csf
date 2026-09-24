# Histórico técnico

Este documento preserva os principais marcos do projeto. Ele não substitui `PROVENANCE.md` nem `VALIDATION.md`.

## 1. Kbase / CSF bring-up

A primeira fase estabeleceu comunicação funcional com a Mali-G720 através de `/dev/mali0`: handshake/UAPI, propriedades, memória, CSG, filas CSF e execução real na GPU.

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

Marcos incluíram `vulkaninfo`, compute, graphics, offscreen/readback, texture, depth, stencil, blending, MSAA e stress.

## 3. WSI / Termux:X11

Foram validados swapchain X11 ARM64, `vkcube` e runs prolongados. Isso comprova o caminho específico, não conformidade Vulkan geral.

## 4. Box64 / Wine

Workloads x86_64/Windows foram conduzidos por Box64/Wine até o loader Vulkan AArch64 e PanVK.

## 5. DXVK G720 LAB

Um bypass experimental permitiu investigar DXVK além do gate inicial de features. O objetivo era diagnóstico, não fingir implementação ausente.

## 6. Investigação de BCn

A leitura de texture feature bits no hardware de referência não anunciou BC1–BC7 no caminho investigado. Surgiu a regra de não anunciar `textureCompressionBC` sem implementação real.

## 7. Wrapper Bionic → glibc

O `bionic-vulkan-wrapper` foi estudado/portado experimentalmente para comparar caminhos indiretos. Com o amadurecimento do PanVK nativo, deixou de ser o caminho principal.

## 8. Transição definitiva para PanVK nativo

A arquitetura foi consolidada em:

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

## 9. Tessellation compiler bring-up

O trabalho incluiu VS → COMPUTE, TCS → COMPUTE, TES → VERTEX, metadata, binding/state, descriptors e poly sysvals.

## 10. Tessellation precompiled kernels

Entre os kernels investigados:

- `panlib_prefix_sum_tess`
- `panlib_tess_isoline`
- `panlib_tess_tri`
- `panlib_tess_quad`

## 11. Tessellation direct-runtime

A validação trabalhou software VS → TCS, libpoly COUNT/prefix/WITH_COUNTS, indexed indirect draw, TES/IDVS, rasterização, `gl_TessCoord`, triangles/quads/isolines, spacing variants, common-edge, winding, stress e CTS focado.

## 12. Geometry Shader

A linha avançou para Geometry Shader com boundaries causais.

O closeout dirigido cobriu:

- semântica GS representativa;
- draws diretos/indiretos;
- Tessellation + GS;
- Transform Feedback + GS;
- Layered GS;
- regressão integrada.

## 13. Consolidação pós-GS

O source técnico foi consolidado e qualificado, preservando proveniência e separando source, build e runtime.

## 14. GPU WAIT64 interno

O commit técnico `980ac91de74df5e5807e6269fd2531fa3ee6b4e5` promoveu o caminho de waits internos na GPU para payloads PanVK elegíveis.

A promoção preservou fallback externo para `sync_file`, timeline, mixed, wait-only e oversized.

## 15. Beta 2

Em 2026-09-24 foi publicada:

- title: `PanVK G720 0.1.0 Beta 2`
- tag: `0.1.0-beta.2`
- package SHA-256: `fc1d69647c071ca3fe30ae2fb450e95c91c08e90779eb32c865fa384dff5aaca`

A partir dessa release o projeto adota sequência SemVer convencional `0.1.0-beta.N`.

## 16. Estado atual

Prioridades após a Beta 2:

- regressões comunitárias;
- Winlator/Vortek/DXVK;
- bugs visuais e sincronização;
- cobertura focada;
- performance somente após correctness.

A release pública atual é `0.1.0-beta.2`.
