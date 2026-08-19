# Mali-G720 Kbase/CSF + PanVK

Projeto experimental para adaptar o Mesa/PanVK à Arm Mali-G720 utilizando diretamente a interface Kbase/CSF presente em kernels Android.

O caminho principal atual é nativo: PanVK fala diretamente com Kbase/CSF. Wrappers, Wine, Box64 e DXVK continuam documentados como provas de conceito e ferramentas de validação, mas não fazem parte da arquitetura interna do driver.

> ⚠️ Projeto experimental e ainda em desenvolvimento. Risco real de GPU faults, travamentos ou necessidade de reinicialização durante testes.

---

## ✅ Hardware / Kernel (validação)

Hardware validado:
- SoC: MediaTek MT6899
- GPU: Mali-G720 MC8
- GPU ID: `0xc8700010`
- Vendor ID: `0x13b5`
- Kbase: r49p1
- UK version: 1.30
- Device: `/dev/mali0`
- Firmware CSF: `mali_csffw.bin`

Observações:
- A UAPI Kbase utilizada pelo Mesa foi comparada com o driver vendor e os ioctl(s) relevantes foram validados.
- Não tratar incompatibilidade de ioctl como hipótese atual — essa hipótese já foi eliminada.

Principais ioctls / operações confirmadas:
- ✅ VERSION_CHECK_CSF
- ✅ SET_FLAGS
- ✅ GPUPROPS
- ✅ CS_GET_GLB_IFACE
- ✅ MEM_ALLOC_EX / BASE_MEM_SAME_VA
- ✅ mmap de BO
- ✅ leitura / escrita CPU ↔ BO
- ✅ MEM_QUERY / MEM_COMMIT
- ✅ criação de CSG
- ✅ registro / bind / kick de filas CSF
- ✅ execução real de comandos na GPU

---

## 🔧 Correção CSF específica para Mali-G720 (mantida)

Problema observado originalmente:
- O backend assumia um valor fixo de work registers:
  ```c
  .cs_reg_count = 96,
  ```
- Isso causava abort com:
  ```
  overflowed register file
  ```

Correção aplicada (conforme já documentado):
- Usar a quantidade real reportada pela interface CSF:
  ```c
  .cs_reg_count = (stream_features & 0xff) + 1,
  ```
- Também ajustar reserva de registradores não preservados:
  ```c
  .nr_kernel_registers =
     MAX2(csif_info->unpreserved_cs_reg_count, 4)
  ```

Na Mali-G720 o campo `stream_features` reportou um valor que implica ~114 registradores e essa correção eliminou o abort "overflowed register file" observado anteriormente.

---

## 🧪 PanVK nativo (estado validado)

Resumo do que já foi validado no PanVK AArch64 nativo sobre Kbase/CSF:
- ✅ vulkaninfo concluído
- ✅ enumeração da Mali-G720
- ✅ compute (workloads compute executando)
- ✅ graphics pipeline
- ✅ render offscreen (triângulo)
- ✅ readback para CPU funcionando
- ✅ texture sampling básico
- ✅ depth D32
- ✅ alpha blending
- ✅ stencil D24S8
- ✅ MSAA 4x + resolve
- ✅ testes repetidos / stress
- ✅ swapchain Vulkan X11 funcionando
- ✅ teste de ~300 frames
- ✅ vkcube via Termux:X11

Observação de arquitetura:
Vulkan application
 -> Mesa/PanVK
 -> Kbase backend
 -> Kbase CSF
 -> Mali-G720

Importante:
- O PanVK usa diretamente `/dev/mali0`.
- Isto é um experimento funcional; NÃO declarar conformidade Vulkan ou suporte completo.

---

## 🧩 Box64 / Wine (experimentação)

Validações realizadas via Box64:
- ✅ Execução de binário Linux x86_64 via Box64.
- ✅ Vulkan x86_64 através do Box64.
- ✅ Swapchain Vulkan X11 x86_64 via Box64.
- ✅ Wine amd64/WOW64 executando através do Box64.
- ✅ Aplicação Windows PE usando Vulkan (pipeline testado).

Caminho verificado (Windows Vulkan via Box64):
Windows PE
 -> Wine (winevulkan)
 -> Box64
 -> Vulkan loader AArch64
 -> PanVK
 -> Kbase CSF
 -> Mali-G720

Aviso:
- Não afirmar que “todos os jogos funcionam”. São provas de conceito e testes limitados.

---

## 🔬 DXVK (seção separada)

DXVK de referência usado: v3.0.2

DXVK stock:
- ❌ O DXVK stock atualmente rejeita o adapter PanVK porque faltam features Vulkan exigidas:
  - geometryShader
  - multiViewport
  - shaderClipDistance
  - shaderCullDistance
  - textureCompressionBC

Importante: NÃO afirmar que essas features já foram implementadas no PanVK.

DXVK G720 LAB (patch experimental):
- Existe um patch experimental chamado "G720 LAB" que contorna a rejeição inicial do adaptador apenas para investigação.
- Esse patch NÃO implementa as features faltantes; ele ignora a checagem inicial para permitir investigação experimental.

Com o G720 LAB observou-se:
- ✅ criação de dispositivo D3D11 (feature level 10_1) em contexto experimental
- ✅ render target offscreen (clear / draw)
- ✅ copy / map / readback
- ✅ execução de shader DXBC (conversão DXBC -> SPIR-V pelo DXVK)
- ✅ execução do shader pelo PanVK
- ✅ Draw(3) + readback correto
- ✅ swapchain em PresentImmediate (~600 frames testados)

Observações:
- FIFO / syncInterval=1 apresentou bloqueio durante testes; Immediate / Present(0) funcionou no experimento.
- NÃO atribuir definitivamente esse problema ao PanVK sem investigação adicional.
- O G720 LAB é uma ferramenta de investigação, não uma solução final.

---

## 🧾 Texture compression (BC) — resultado da investigação

Medida observada:
- Valor real observado de TEXTURE_FEATURES_0: `0xc7fe001e`
- Máscara esperada para formatos BC: `0x0001ff80`
- Interseção observada: `0x00000000`

Conclusão:
- A Mali-G720, no caminho Kbase/CSF observado, NÃO anuncia suporte nativo a BC1–BC7 via esses texture feature bits.
- O fato do GameNative/driver proprietário anunciar `textureCompressionBC` não prova suporte BC nativo do hardware — pode haver uma camada de wrapper ou emulação.
- NÃO forçar `textureCompressionBC=true` no PanVK sem implementação real e verificação.

---

## 🎮 GameNative / wrapper (controle positivo)

No mesmo hardware, abordagens proprietárias (GameNative / Winlator / driver vendor) conseguem executar workloads que anunciam:
- geometryShader
- tessellationShader
- multiViewport
- shaderClipDistance
- shaderCullDistance
- textureCompressionBC

Esses caminhos frequentemente usam um wrapper/proprietary driver que pode:
- virtualizar/transformar features
- emular BCn (decompress/transcode via CPU ou compute)
- remover/baixar (lower) ClipDistance/CullDistance
- transformar SPIR-V

Análise do wrapper público estudado:
- Projeto: leegao/bionic-vulkan-wrapper
- O wrapper público implementa, entre outras coisas:
  - virtualização de algumas Vulkan features
  - emulação de BCn (BC1/BC2/BC3/BC4/BC5/BC6H/BC7)
  - decompression/transcode por CPU ou compute
  - transformação e lowering de SPIR-V relacionados a Clip/Cull
- NÃO assumir que o wrapper implementa geometry shader ou multiViewport completos sem evidências técnicas suficientes.

---

## 🧩 Wrapper glibc experimental

Objetivo:
Portar o bionic-vulkan-wrapper para AArch64 com glibc para compor esta cadeia experimental:
DXVK / Vulkan loader
 -> libvulkan_wrapper.so
 -> libvulkan_panfrost.so
 -> Kbase
 -> Mali-G720

Progresso e problemas corrigidos (Bionic → glibc):
- ✅ Android availability macros em stubs
- ✅ C11 threads / HAVE_THRD_CREATE
- ✅ once_flag
- ✅ memfd_create
- ✅ getrandom
- ✅ correções de const correctness com Clang 21
- ✅ cnd_monotonic
- ✅ u_printf
- ✅ remoção do WSI AHardwareBuffer no build X11
- ✅ flags fcntl/open
- ✅ buffer_handle_t
- ✅ getprogname → program_invocation_short_name
- ✅ size_t em spirv_edit.h

Estado histórico:
- O snapshot preservado em `porting/g720-wrapper-glibc-snapshot/` representa um checkpoint intermediário do port Bionic → glibc.
- Nesse checkpoint, a compilação havia chegado ao link final de `libvulkan_wrapper.so`, ainda bloqueado por passes customizados ausentes no SPIRV-Tools.
- O trabalho com o wrapper foi usado como PoC para estudar o caminho Vulkan e comparar features virtualizadas.
- O caminho principal atual não depende mais desse wrapper: PanVK executa diretamente sobre Kbase/CSF.

O snapshot permanece no repositório para preservar o histórico da investigação, não como arquitetura recomendada atualmente.

---

## 🧪 PoCs / marcos históricos preservados

As provas de conceito abaixo registram etapas diferentes do desenvolvimento. Elas não significam conformidade Vulkan nem compatibilidade geral com jogos.

1. **Kbase/CSF bring-up**
   - comunicação com `/dev/mali0`
   - UAPI e propriedades da GPU
   - memória e BOs
   - CSG, filas CSF e execução real na GPU

2. **PanVK Vulkan nativo**
   - vulkaninfo
   - compute
   - graphics pipeline
   - triângulo offscreen + readback
   - texture sampling
   - depth/stencil
   - blending
   - MSAA + resolve
   - stress

3. **Termux:X11 / WSI**
   - swapchain Vulkan ARM64
   - vkcube
   - teste prolongado de aproximadamente 300 frames

4. **Box64 / Wine Vulkan**
   - execução x86_64
   - Wine amd64/WOW64
   - aplicação Windows chegando ao PanVK por Wine/Box64

5. **DXVK G720 LAB**
   - criação experimental de dispositivo D3D11
   - clear/draw
   - copy/map/readback
   - DXBC → SPIR-V
   - Draw(3)
   - PresentImmediate

6. **Wrapper Bionic → glibc**
   - PoC histórica para estudar a rota indireta:
     `wrapper → PanVK → Kbase/CSF → G720`
   - checkpoint preservado em `porting/g720-wrapper-glibc-snapshot/`

7. **Transição para PanVK nativo**
   - remoção do wrapper do caminho principal
   - caminho atual:
     `Vulkan → PanVK → Kbase/CSF → Mali-G720`

8. **Tessellation compiler bring-up**
   - VS → COMPUTE
   - TCS → COMPUTE
   - TES → VERTEX
   - metadata, binding/state, descriptors e poly sysvals

9. **Tessellation precompiled kernels**
   - `panlib_prefix_sum_tess`
   - `panlib_tess_isoline`
   - `panlib_tess_tri`
   - `panlib_tess_quad`

10. **Tessellation direct-runtime groundwork**
   - poly heap por command buffer
   - buffers e parâmetros libpoly por draw
   - `poly_vertex_params` / `poly_tess_params`
   - `gfx.sysvals.poly`
   - TLS incluindo TCS/TES físicos
   - software VS executado como compute
   - dependência CSF WAIT
   - TCS executado como compute
   - checkpoint de código: `0a742fa22ec`

---

## 🧬 Tessellation (desenvolvimento atual)

`tessellationShader` continua **desativado**. A implementação atual ainda não
representa tessellation funcional completa e a feature não será anunciada antes
de validação real em hardware.

### ✅ Infraestrutura concluída / validada por build

Compiler e ABI:
- ✅ VS → COMPUTE
- ✅ TCS → COMPUTE
- ✅ TES → VERTEX
- ✅ metadata TCS/TES
- ✅ binding/state TCS/TES
- ✅ descriptors TCS/TES
- ✅ poly sysvals ABI
- ✅ auditoria do ABI libpoly
- ✅ correção Physical64 `ptr_size`

Kernels:
- ✅ `panlib_prefix_sum_tess`
- ✅ `panlib_tess_isoline`
- ✅ `panlib_tess_tri`
- ✅ `panlib_tess_quad`
- ✅ kernels limitados às arquiteturas CSF v10/v12/v13/v14

Runtime direto já integrado em código:
- ✅ poly heap por command buffer
- ✅ buffers por draw para VS/TCS/tessellation
- ✅ `poly_vertex_params`
- ✅ `poly_tess_params`
- ✅ preservação da máscara original de outputs do software VS
- ✅ `gfx.sysvals.poly`
- ✅ TLS considerando TCS e TES físicos
- ✅ desvio do caminho tessellation antes do IDVS normal
- ✅ software VS disparado pelo mecanismo compute existente do PanVK
- ✅ CSF WAIT entre software VS e TCS
- ✅ TCS disparado pelo mecanismo compute existente do PanVK

Checkpoint atual:
- `85b972dd387` — per-command-buffer poly heap
- `0a742fa22ec` — direct tessellation SW VS → WAIT → TCS

As arquiteturas legadas v4/v5/v6/v7/v9 continuam usando o conjunto normal de
shaders libpan sem os kernels CSF de tessellation.

### 🚧 Runtime ainda pendente

- lançar o processamento do tessellator / contagem
- executar prefix sum e alocar/preencher o index buffer final
- disparar o kernel libpoly correspondente à topologia
- executar TES e consumir o indexed draw gerado
- validar o caminho direct completo na Mali-G720
- implementar/validar tessellation indirect
- garantir execução simultânea segura do mesmo command buffer de tessellation
- executar regressões e validação de feature antes de anunciar suporte

Portanto, o checkpoint atual comprova a integração de compilador, ABI,
recursos e a sequência física **SW VS → WAIT → TCS** no build. Ainda não
comprova renderização tessellated completa.

`tessellationShader=true` só será anunciado depois da implementação completa e
validação em hardware real.

---

## 🛠️ Próximos passos (priorizados)

A prioridade continua sendo completar o caminho PanVK nativo sem criar uma
segunda infraestrutura de dispatch ou espalhar dependências kbase pelas
camadas genéricas.

1. Integrar o lançamento do tessellator e a etapa de contagem. (🚧)
2. Integrar prefix sum e geração/alocação do index buffer. (🚧)
3. Disparar o kernel libpoly de isolines/triangles/quads apropriado. (🚧)
4. Integrar TES e o indexed draw final gerado pelo tessellator. (🚧)
5. Validar tessellation direct completa na Mali-G720 real. (🚧)
6. Implementar e validar o caminho indirect. (🚧)
7. Resolver segurança de simultaneous-use do mesmo command buffer. (🚧)
8. Só então anunciar `tessellationShader`. (🚧)
9. Depois disso, fazer uma auditoria focada de vazamentos panthor/DRM/UAPI
   fora da camada kmod e reduzir diferenças desnecessárias do upstream. (🧪)
10. Continuar depois a investigação de `geometryShader` e `multiViewport`. (🧪)
11. Manter `textureCompressionBC` desativado sem implementação real. (🚧)
12. Revalidar DXVK/VKD3D sobre uma base PanVK estável. (🧪)

Política de manutenção do fork:
- manter PanVK/libpoly genéricos o mais próximos possível do upstream;
- isolar detalhes kbase/CSF na camada mais baixa apropriada;
- não anunciar features antes de implementação e validação;
- quando o Mesa upstream implementar uma solução equivalente, preferir a
  implementação upstream em vez de manter uma duplicação local.

Status legend:
- ✅ integrado / validado no nível indicado
- 🧪 experimental / investigação
- 🚧 em desenvolvimento
- ❌ ainda não suportado / não implementado

---

## 🙏 Créditos (preservados)

- Leegao — https://github.com/leegao  
  Autor do fork usado como base: https://github.com/leegao/mesa-funnymdzz

- funnymdzz — https://github.com/funnymdzz  
  Trabalho base: https://github.com/funnymdzz/mesa

- Icecream95  
  Pelo trabalho pioneiro relacionado ao Panfork/Panfrost e à engenharia reversa de GPUs Mali e Kbase.

- Mesa / Panfrost / PanVK  
  Pelo desenvolvimento open-source da infraestrutura e do compilador utilizados neste projeto.

- Saikatsaha1996 / mesa-Panfrost-G610  
  Pelas referências e contribuições comunitárias envolvendo Mali G610/G710 e CSF.

- wonderkast02  
  Desenvolvimento e validação em:
  - MediaTek MT6899
  - Mali-G720
  - Kbase r49p1
  - Engenharia reversa do driver vendor
  - Validação da UAPI e testes CSF
  - Adaptação do backend Kbase
  - Correção do CS register count para G720
  - Testes e documentação


---

## ⚠️ Aviso final

Projeto experimental de pesquisa e engenharia reversa.

O código e as ferramentas aqui documentadas podem causar GPU faults, travamentos ou exigir reinicialização durante o desenvolvimento. Testes em hardware real devem ser feitos com cautela.
