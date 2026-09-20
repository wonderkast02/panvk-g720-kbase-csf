# Matriz de validação

A validação do Drive G720 é baseada em evidência física e escopo explícito. Resultado em um caso não é automaticamente generalizado para outro hardware, runtime ou combinação de features.

## Plataforma autoritativa principal

- SoC: MediaTek MT6899
- GPU: Mali-G720 MC8
- GPU ID: `0xc8700010`
- Vendor ID: `0x13b5`
- Kbase / CSF
- Android

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

Evidências históricas preservadas incluem:

- Direct/P7/Replay: 9/9;
- common-edge triangles: 6/6;
- common-edge quads: 6/6;
- winding: 48/48;
- domain-origin: 4 PASS / 8 NOT_SUPPORTED / 0 FAIL;
- simultaneous-use sync64: 64/64;
- DYN256: 19/19;
- stress até 8192 triangle patches;
- duas repetições de CTS focado: 160 PASS / 954 NOT_SUPPORTED / 0 FAIL / 0 OTHER.

Esses números são evidência dirigida, não claim de conformidade Vulkan.

## Geometry Shader — linha pós-beta

Após a beta pública `0.1.0-beta.1.9.4`, o projeto fechou funcionalmente o escopo dirigido de Geometry Shader no hardware de referência.

O closeout incluiu, dentro do escopo testado:

- criação e execução de pipelines GS;
- topologias e semânticas representativas;
- draws diretos e indiretos representativos;
- composição Tessellation + GS;
- Transform Feedback + GS em stream suportado;
- layered GS;
- regressão integrada do caminho gráfico relevante.

Casos bloqueados por transporte, tooling ou instrumentação são classificados separadamente e não viram automaticamente falha PanVK.

A evidência pós-beta não altera retroativamente o conteúdo da release pública atual.

## FullPlane + O_RDONLY — beta pública

A qualificação nativa usada na release pública passou as matrizes exatas registradas para control/candidate/bridge/core:

- control: 10/10
- candidate: 10/10
- bridge: 18/18
- core: 18/18

## Winlator / Vortek — release pública

No estado congelado de `0.1.0-beta.1.9.4`, o run registrado chegou ao primeiro `vkQueueSubmit`, mas não chegou a acquire/present.

O `_wassert` observado nesse run **não prova sozinho GPU fatal**.

Desenvolvimento posterior pode divergir desse estado; resultados devem sempre registrar pacote, SHA-256 e runtime exatos.

## Regras de interpretação

- runtime > grep;
- evidência bruta > classificador;
- build pass != runtime pass;
- falha de instrumentação != falha do driver;
- transporte indisponível != falha do driver;
- um resultado em MC8 != claim universal de G720;
- nenhuma seção deste documento declara conformidade Vulkan.
