# Compatibilidade — Winlator, DXVK, Wine, Box64 e wrappers

Este documento preserva experimentos históricos e fronteiras de compatibilidade sem misturá-los à arquitetura interna do PanVK.

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

O projeto usou DXVK em diferentes fases.

Um estado histórico de DXVK stock rejeitava o adapter PanVK por ausência de features Vulkan exigidas naquele momento, incluindo:

- `geometryShader`
- `multiViewport`
- `shaderClipDistance`
- `shaderCullDistance`
- `textureCompressionBC`

Esse registro descreve aquele ponto do desenvolvimento. Features implementadas posteriormente devem ser avaliadas pelo estado atual do driver, sem reescrever o contexto histórico.

### G720 LAB

Foi usado um patch experimental de investigação denominado **G720 LAB** para contornar a rejeição inicial do adapter.

O patch **não implementava** as features faltantes; ele permitia investigar o que acontecia após o gate inicial.

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

O fato de um driver proprietário ou wrapper anunciar `textureCompressionBC` não prova suporte nativo de hardware no PanVK. Pode haver emulação, decompression/transcode ou outra camada de compatibilidade.

Portanto:

> Não forçar `textureCompressionBC=true` sem implementação real e validação.

## Driver proprietário / GameNative como controle positivo

Caminhos proprietários ou wrappers no mesmo hardware podem anunciar features que não correspondem diretamente ao contrato nativo do PanVK.

Essas camadas podem, por exemplo:

- virtualizar features;
- transformar SPIR-V;
- emular formatos;
- decomprimir/transcodificar BCn;
- fazer lowering de ClipDistance/CullDistance.

Feature observada em wrapper não deve ser copiada para o PanVK apenas por advertisement.

## `bionic-vulkan-wrapper`

O wrapper público estudado historicamente foi usado para compreender:

- virtualização de algumas features Vulkan;
- caminhos de emulação BCn;
- transformação de SPIR-V;
- lowering relacionado a Clip/Cull.

Não assumir que um wrapper implementa integralmente GS, multiview, multiViewport ou qualquer outra feature sem evidência específica.

## Port Bionic → glibc

Houve uma PoC para portar o wrapper para AArch64/glibc:

```text
DXVK / Vulkan loader
       ↓
libvulkan_wrapper.so
       ↓
libvulkan_panfrost.so
       ↓
Kbase
       ↓
Mali-G720
```

Problemas históricos tratados nessa frente incluíram:

- Android availability macros;
- C11 threads / `HAVE_THRD_CREATE`;
- `once_flag`;
- `memfd_create`;
- `getrandom`;
- const correctness com Clang;
- `cnd_monotonic`;
- `u_printf`;
- WSI/AHardwareBuffer no build X11;
- `fcntl/open`;
- `buffer_handle_t`;
- `getprogname`;
- `size_t` em `spirv_edit.h`.

O snapshot histórico chegou ao link final de `libvulkan_wrapper.so`, ainda com dependência de passes customizados do SPIRV-Tools naquele checkpoint.

O wrapper deixou de ser arquitetura recomendada quando o caminho PanVK nativo se tornou o foco principal.

## Winlator / Vortek

Winlator e Vortek são ambientes de integração/compatibilidade.

Na beta pública `0.1.0-beta.1.9.4`, o run congelado chegou ao primeiro `vkQueueSubmit`, mas não chegou a acquire/present.

Desenvolvimento posterior deve ser comparado usando bytes e hashes exatos; não misture o estado da beta pública com builds pós-beta.

## Regra causal

Ao diagnosticar compatibilidade:

- falha de aplicação != falha do driver automaticamente;
- falha de wrapper != falha PanVK automaticamente;
- falha de transporte != falha PanVK automaticamente;
- renderização incorreta invalida qualquer claim de ganho de performance;
- correctness vem antes de FPS.
