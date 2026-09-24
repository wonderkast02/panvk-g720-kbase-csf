# Matriz de validação

A validação do Drive G720 é baseada em evidência física e escopo explícito. Resultado em um caso não é automaticamente generalizado para outro hardware, runtime ou combinação de features.

## Plataforma autoritativa principal

- SoC: MediaTek MT6899
- GPU: Mali-G720 MC8
- GPU ID: `0xc8700010`
- Vendor ID: `0x13b5`
- Kbase / CSF
- Android

## Beta 2 — identidade qualificada

- tag: `0.1.0-beta.2`
- package SHA-256: `fc1d69647c071ca3fe30ae2fb450e95c91c08e90779eb32c865fa384dff5aaca`
- embedded SO SHA-256: `126b8b6124a8677298469f520cd6c825de88b498fe93a38bd358ef3b557882d3`
- technical source authority: `980ac91de74df5e5807e6269fd2531fa3ee6b4e5`

## PanVK nativo

Validado no escopo registrado:

- execução direta sobre Kbase/CSF;
- compute e graphics;
- render offscreen e readback;
- texture sampling;
- depth, stencil e blending;
- MSAA + resolve;
- X11 WSI / swapchain;
- Android AHB import;
- AIMapper v5 FullPlane metadata;
- regressões de coerência GPU/CPU.

## Tessellation

A linha MC8 autoritativa validou `tessellationShader=true` em hardware real.

Evidências dirigidas preservadas incluem:

- Direct/P7/Replay: 9/9;
- common-edge triangles: 6/6;
- common-edge quads: 6/6;
- winding: 48/48;
- domain-origin: 4 PASS / 8 NOT_SUPPORTED / 0 FAIL;
- simultaneous-use sync64: 64/64;
- DYN256: 19/19;
- stress até 8192 triangle patches;
- duas repetições de CTS focado: 160 PASS / 954 NOT_SUPPORTED / 0 FAIL / 0 OTHER;
- semantic framebuffer oracle;
- triangle, quad e isoline point-mode;
- fractional-even e fractional-odd no escopo dirigido.

Esses números são evidência dirigida, não claim de conformidade Vulkan.

## Geometry Shader

A Beta 2 parte da linha que fechou funcionalmente o escopo dirigido de Geometry Shader no hardware de referência.

O closeout incluiu:

- criação e execução de pipelines GS;
- topologias e semânticas representativas;
- draws diretos e indiretos representativos;
- composição Tessellation + GS;
- Transform Feedback + GS em stream suportado;
- layered GS;
- regressão integrada do caminho gráfico relevante.

## GPU WAIT64 interno

A autoridade técnica Beta 2 qualifica o caminho de espera GPU para dependências binárias locais/internas elegíveis.

Boundary preservado:

- eligible internal/local binary payload: GPU WAIT64;
- imported `sync_file`: fallback;
- timeline wrapper: fallback;
- mixed wait set: fallback;
- wait-only submit: fallback;
- oversized wait set: fallback.

Isso não substitui a sincronização externa.

## Build / package qualification

Para a autoridade técnica da Beta 2:

- target build: PASS;
- full build: PASS;
- Meson test: PASS;
- ELF AArch64: PASS;
- SONAME `libvulkan_panfrost.so`: PASS;
- NEEDED `libdrm`: NO;
- GLIBC version refs: NO;
- undefined `panthor`: NO;
- package determinístico: PASS;
- asset upload/download roundtrip: PASS.

## Winlator / Vortek / DXVK

Essas camadas continuam experimentais.

A Beta 2 deve ser reportada sempre com tag e SHA exatos. Um bug observado em Winlator/DXVK não é automaticamente atribuído ao PanVK sem causalidade suficiente.

O run histórico da Beta 1.9.4 que chegou ao primeiro `vkQueueSubmit` mas não a acquire/present permanece documentado como **histórico**, não como estado atual da Beta 2.

## Regras de interpretação

- runtime > grep;
- evidência bruta > classificador;
- build pass != runtime pass;
- falha de instrumentação != falha do driver;
- transporte indisponível != falha do driver;
- um resultado em MC8 != claim universal de G720;
- correctness vem antes de FPS;
- nenhuma seção deste documento declara conformidade Vulkan.
