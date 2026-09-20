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

## Fase atual

- [ ] consolidar árvore pós-GS;
- [ ] garantir que o diff final contenha apenas código de produção;
- [ ] preservar proveniência de fonte/build/runtime;
- [ ] melhorar reprodutibilidade;
- [ ] ampliar regressões sem reabrir casos fechados sem causa real.

## Compatibilidade

- [ ] fortalecer Winlator / Vortek;
- [ ] reavaliar DXVK com o contrato de features atual;
- [ ] validar Wine / Box64 e workloads Windows de forma dirigida;
- [ ] separar bugs de driver, wrapper, transporte e aplicação.

## Cobertura

- [ ] ampliar CTS focado;
- [ ] aumentar matriz de regressão;
- [ ] testar outras variantes/dispositivos Mali-G720;
- [ ] documentar diferenças de Kbase/firmware/UAPI quando aparecerem.

## Performance

Performance vem depois da correção funcional:

- [ ] profiling;
- [ ] eliminar gargalos comprovados;
- [ ] reduzir overhead;
- [ ] comparar regressões com baseline controlado;
- [ ] medir jogos somente quando renderização estiver correta.

## Releases

Nenhuma nova beta é criada por avanço interno isolado. Release pública exige candidato congelado, proveniência, hashes, qualificação e autorização explícita.
