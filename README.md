# Mali-G720 Kbase/CSF + PanVK

Projeto experimental para adaptar o **Mesa/PanVK** à **Arm Mali-G720** utilizando diretamente a interface **Kbase/CSF** presente em kernels Android.

## 🎯 Hardware testado

- **SoC:** MediaTek MT6899
- **GPU:** Mali-G720 MC8
- **GPU ID:** `0xc8700010`
- **Vendor ID:** `0x13b5`
- **Kbase:** r49p1
- **UK version:** 1.30
- **Device:** `/dev/mali0`

> ⚠️ Projeto experimental e ainda em desenvolvimento.

---

## 🚀 Estado atual

### Kbase / CSF

- ✅ Acesso funcional ao `/dev/mali0`
- ✅ UK version **1.30** confirmada
- ✅ Propriedades reais da GPU obtidas
- ✅ UAPI Kbase **r49p1** validada
- ✅ `CS_GET_GLB_IFACE` funcionando
- ✅ `MEM_ALLOC_EX` / `BASE_MEM_SAME_VA`
- ✅ mmap CPU/GPU funcionando
- ✅ Criação de CSG e filas CSF
- ✅ Queue bind / kick funcionando
- ✅ Interfaces CSF mapeadas
- ✅ Emissão de comandos CSF reais
- ✅ Escrita real em memória pela GPU
- ✅ Cópia real de memória pela GPU

---

## Mesa / PanVK

A etapa atual da integração utiliza como principal base:

- **Leegao:** https://github.com/leegao/mesa-funnymdzz
- **funnymdzz:** https://github.com/funnymdzz/mesa

O fork do **Leegao** trouxe uma implementação muito mais completa de PanVK sobre Kbase e permitiu avançar significativamente a adaptação para a Mali-G720.

### Avanços comprovados

- ✅ Mesa/PanVK compilado nativamente em AArch64
- ✅ Backend Kbase habilitado
- ✅ Backends `panfrost`, `panthor` e `kbase` coexistindo
- ✅ PanVK encontra diretamente `/dev/mali0`
- ✅ Mali-G720 identificada corretamente
- ✅ BOs e ringbuffers Kbase funcionando
- ✅ Subqueues CSF 0/1/2 funcionando
- ✅ `seqno` chegando ao valor esperado
- ✅ Espera das filas concluindo corretamente
- ✅ `vulkaninfo --summary` concluído com **exit 0**

Resultado real:

```text
Found kbase device '/dev/mali0'.

panvk (Mali-G720 MC8)

apiVersion = 1.4.354
vendorID   = 0x13b5
deviceID   = 0xc8700010
deviceType = PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
deviceName = Mali-G720 MC8
driverName = panvk
```

---

## 🔧 Correção descoberta para Mali-G720

O backend Kbase usado como base assumia inicialmente:

```c
.cs_reg_count = 96,
```

Na Mali-G720 isso fazia o PanVK abortar com:

```text
overflowed register file
```

A correção foi utilizar a quantidade de work registers anunciada pela própria interface CSF:

```c
.cs_reg_count = (stream_features & 0xff) + 1,
```

Também foi configurada explicitamente a reserva dos registradores não preservados pelo firmware:

```c
.nr_kernel_registers =
   MAX2(csif_info->unpreserved_cs_reg_count, 4),
```

Após a correção:

```text
BUILD OK
vulkaninfo exit=0
deviceName = Mali-G720 MC8
driverName = panvk
```

Também foi observado progresso real das filas CSF:

```text
extract=320
ls_copy=1
cell->seqno=1
target_seqno=1
```

e:

```text
kbase_queue_wait_current: wait completed successfully
```

Isso mostra que o trabalho já ultrapassou a simples detecção da GPU e chegou à execução e sincronização das filas CSF.

---

## 🧪 Proof of Concept original

O repositório também preserva os testes usados durante a primeira etapa da engenharia reversa.

| Arquivo | Descrição |
|---|---|
| `libkbase_csf.c/.h` | Inicialização, memória e submissão Kbase/CSF |
| `test_gpu_write.c` | GPU escreve `0xDEADBEEF` em memória |
| `test_gpu_copy_v2.c` | Cópia de memória executada pela GPU |
| `test_backend.c` | Testes do backend experimental |
| `pan_kmod_kbase.c` | Primeiro backend experimental Mesa/Kbase |
| `include/` | Headers da UAPI Kbase |

---

## 🛣️ Próximos passos

- Testar estabilidade sem `PANVK_DEBUG=kbase_diag`
- Executar workloads Vulkan/compute reais
- Validar execução de shaders
- Testar renderização offscreen
- Investigar sincronização sob carga
- Identificar e corrigir possíveis GPU faults
- Evoluir para Android / Winlator
- Consolidar o backend Kbase para Mali-G720

---

## 🙏 Créditos

### Leegao

**https://github.com/leegao**

Autor do fork:

**https://github.com/leegao/mesa-funnymdzz**

Principal base usada na etapa atual da integração **PanVK sobre Kbase**.

### funnymdzz

**https://github.com/funnymdzz**

Autor do trabalho base:

**https://github.com/funnymdzz/mesa**

Trabalho focado em PanVK sobre Kbase em containers Linux/Android.

### Icecream95

Pelo trabalho pioneiro relacionado ao Panfork/Panfrost e à engenharia reversa de GPUs Mali e Kbase.

### Mesa / Panfrost / PanVK

Pelo desenvolvimento open-source da infraestrutura e do compilador utilizados neste projeto.

### Saikatsaha1996 / mesa-Panfrost-G610

Pelas referências e contribuições comunitárias envolvendo Mali G610/G710 e CSF.

### wonderkast02

Desenvolvimento e validação em:

- MediaTek MT6899
- Mali-G720
- Kbase r49p1
- Engenharia reversa do driver vendor
- Validação da UAPI
- Testes CSF
- Adaptação do backend Kbase
- Correção do CS register count para G720
- Testes e documentação

---

## ⚠️ Aviso

Projeto experimental de pesquisa e engenharia reversa.

O código ainda pode causar GPU faults, travamentos ou exigir reinicialização durante o desenvolvimento.
