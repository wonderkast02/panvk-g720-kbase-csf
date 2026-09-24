# Compatibilidade — Winlator, DXVK, Wine, Box64 e wrappers

Este documento preserva experimentos históricos e fronteiras de compatibilidade sem misturá-los à arquitetura interna do PanVK.

## Estado público atual

A release pública atual é `0.1.0-beta.2`.

- package SHA-256: `fc1d69647c071ca3fe30ae2fb450e95c91c08e90779eb32c865fa384dff5aaca`
- Geometry Shader e Tessellation fazem parte do estado de feature atual da linha Beta 2;
- Winlator, Vortek, Wine, Box64, DXVK e VKD3D permanecem **camadas externas experimentais**;
- compatibilidade com jogos deve ser avaliada por runtime/versão/cenário, não inferida do feature table.

## Box64 / Wine

Validações históricas incluíram:

- execução de binário Linux x86_64 via Box64;
- Vulkan x86_64 através do Box64;
- swapchain Vulkan X11 x86_64;
- Wine amd64/WOW64 via Box64;
- aplicação Windows PE usando Vulkan até o PanVK.

Caminho representativo:

```text
Windows PE
   ↓
Wine / winevulkan
   ↓
Box64
   ↓
Vulkan loader AArch64
   ↓
PanVK
   ↓
Kbase / CSF
   ↓
Mali-G720
```

Esses resultados são provas de conceito e não equivalem a compatibilidade universal com aplicações ou jogos.

## DXVK — investigação histórica

Um estado histórico de DXVK stock rejeitava o adapter PanVK por ausência de features Vulkan exigidas naquele momento, incluindo:

- `geometryShader`
- `multiViewport`
- `shaderClipDistance`
- `shaderCullDistance`
- `textureCompressionBC`

Esse registro descreve aquele ponto do desenvolvimento. A Beta 2 já anuncia `geometryShader`, mas isso não implica que todos os requisitos de toda versão do DXVK estejam satisfeitos.

### G720 LAB

Foi usado um patch experimental de investigação para contornar a rejeição inicial do adapter.

O patch **não implementava** as features faltantes; permitia investigar o comportamento após o gate inicial.

Naquele experimento foram observados:

- criação de dispositivo D3D11 em contexto experimental;
- render target offscreen;
- clear/draw;
- copy/map/readback;
- DXBC → SPIR-V via DXVK;
- execução de shader pelo PanVK;
- `Draw(3)` com readback;
- swapchain em PresentImmediate em teste prolongado.

Problemas observados em FIFO/syncInterval=1 não foram atribuídos definitivamente ao PanVK sem evidência causal suficiente.

## Texture compression BC

Em investigação histórica no hardware de referência:

- `TEXTURE_FEATURES_0`: `0xc7fe001e`
- máscara usada na investigação BC: `0x0001ff80`
- interseção observada: `0x00000000`

Conclusão no escopo daquela medição: o caminho Kbase/CSF observado não anunciava BC1–BC7 através desses bits.

> Não forçar `textureCompressionBC=true` sem implementação real e validação.

## Driver proprietário / wrappers como controle

Caminhos proprietários ou wrappers podem:

- virtualizar features;
- transformar SPIR-V;
- emular formatos;
- decomprimir/transcodificar BCn;
- fazer lowering de ClipDistance/CullDistance.

Feature observada em wrapper não deve ser copiada para o PanVK apenas por advertisement.

## `bionic-vulkan-wrapper`

O wrapper público estudado historicamente foi usado para compreender virtualização/emulação de features e transformação SPIR-V.

O wrapper deixou de ser arquitetura recomendada quando o caminho PanVK nativo se tornou o foco principal.

## Winlator / Vortek

Winlator e Vortek são ambientes de integração/compatibilidade.

### Beta 2

Resultados comunitários da Beta 2 devem registrar:

- tag `0.1.0-beta.2`;
- package SHA exato;
- versão Winlator/Vortek;
- DXVK/VKD3D;
- resolução/configuração;
- logs e frametime quando relevante.

Artefatos visuais ou quedas de FPS precisam de reprodução causal antes de serem atribuídos ao PanVK.

### Beta 1.9.4 — histórico

No run congelado histórico de `0.1.0-beta.1.9.4`, a execução chegou ao primeiro `vkQueueSubmit`, mas não a acquire/present.

Esse dado continua válido apenas para os bytes daquela release histórica.

## Regra causal

- falha de aplicação != falha do driver automaticamente;
- falha de wrapper != falha PanVK automaticamente;
- falha de transporte != falha PanVK automaticamente;
- renderização incorreta invalida claim de ganho de performance;
- correctness vem antes de FPS.
