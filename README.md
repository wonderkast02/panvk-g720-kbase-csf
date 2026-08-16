# Mali-G720 Kbase/CSF + PanVK

Projeto experimental para adaptar o Mesa/PanVK à Arm Mali-G720 utilizando diretamente a interface Kbase/CSF presente em kernels Android.

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

Estado atual:
- A compilação chegou ao link final de `libvulkan_wrapper.so`.
- O link está atualmente bloqueado porque `spirv_edit.cpp` referencia passes customizados do SPIRV-Tools que não existem no archive compilado:
  - CreateRemoveClipCullDistPass()
  - CreateFixMaliSpecConstantCompositePass()
  - CreateMaliOptimizationBarrierPass()

Consequência:
- O Wrapper glibc permanece WIP.
- NÃO afirmar que Wrapper → PanVK já foi validado.
- NÃO afirmar que DXVK stock já funciona através deste Wrapper.

---

## 🛠️ Próximos passos (priorizados)

Removidos itens já concluídos das listas antigas. Prioridade atual:
1. Resolver/prover os passes SPIRV-Tools específicos usados pelo bionic-vulkan-wrapper. (🚧)
2. Finalizar o link glibc de `libvulkan_wrapper.so`. (🚧)
3. Testar carregamento direto: `libvulkan_wrapper.so -> libvulkan_panfrost.so`. (🚧)
4. Enumerar a Mali-G720 através do Wrapper e auditar comportamento. (🧪)
5. Auditar as features Vulkan virtualizadas pelo Wrapper (o que é emulação vs. hardware). (🧪)
6. Testar DXVK stock sem o G720 LAB bypass (avaliar rejeições de features). (🚧)
7. Repetir D3D11 offscreen e swapchain com abordagem limpa. (🧪)
8. Somente depois de estabilizar, testar jogos reais com controle experimental. (🚧)
9. Investigar adequadamente geometryShader e multiViewport (evitar apenas alterar feature bits). (🚧)
10. Avaliar integração sustentável de emulação BCn / Clip / Cull (se necessário). (🚧)

Status legend:
- ✅ validado
- 🧪 experimental (provas de conceito)
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
