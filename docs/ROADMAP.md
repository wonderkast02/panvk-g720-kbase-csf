# Roadmap

Este roadmap descreve direção técnica, não promessa de prazo ou de versão pública.

## Concluído no escopo atual

- [x] Kbase / CSF bring-up
- [x] enumeração e propriedades da Mali-G720
- [x] memória/BO e execução GPU
- [x] Vulkan bring-up
- [x] compute
- [x] graphics
- [x] render offscreen + readback
- [x] texture / depth / stencil / blending
- [x] MSAA + resolve
- [x] X11 WSI / swapchain
- [x] Tessellation dirigida
- [x] Geometry Shader dirigido
- [x] Tessellation + GS
- [x] Transform Feedback + GS no escopo qualificado
- [x] Layered GS no escopo qualificado
- [x] consolidação pós-GS em `g720-development`
- [x] build limpo + Meson test da autoridade técnica atual
- [x] promoção do GPU WAIT64 interno
- [x] Beta 2 qualificada e publicada
- [x] documentação da branch de desenvolvimento sincronizada após Beta 2

## Fase atual — regressões e compatibilidade

- [ ] triar logs da comunidade por pacote/runtime/hardware;
- [ ] reproduzir artefatos visuais e regressões de frametime;
- [ ] separar driver, DXVK, runtime, transporte e aplicação;
- [ ] corrigir bugs com primeiro-fail causal;
- [ ] ampliar regressões sem reabrir casos fechados sem causa real.

## Compatibilidade

- [ ] fortalecer Winlator / Vortek;
- [ ] reavaliar DXVK com o contrato de features atual;
- [ ] validar Wine / Box64 e workloads Windows de forma dirigida;
- [ ] documentar matriz por runtime e versão;
- [ ] manter correctness antes de performance.

## Cobertura

- [ ] ampliar CTS focado;
- [ ] aumentar matriz de regressão;
- [ ] testar outras variantes/dispositivos Mali-G720;
- [ ] documentar diferenças de Kbase/firmware/UAPI quando aparecerem.

## Performance

Somente após renderização correta e sincronização estável:

- [ ] profiling;
- [ ] eliminar gargalos comprovados;
- [ ] reduzir overhead;
- [ ] comparar regressões com baseline controlado;
- [ ] medir jogos com cenário reproduzível.

## Releases

Release pública atual: `0.1.0-beta.2`.

A próxima beta convencional é `0.1.0-beta.3`, mas só será criada se houver:

1. candidato público congelado;
2. proveniência e hashes;
3. qualificação suficiente;
4. decisão explícita de lançamento.
