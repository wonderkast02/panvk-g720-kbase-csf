<div align="center">

# PanVK para Mali-G720

### Vulkan experimental sobre Kbase/CSF no Android

**`Vulkan → Mesa/PanVK → Kbase/CSF → Mali-G720`**

<br>

![Status](https://img.shields.io/badge/STATUS-PUBLIC_BETA-F59E0B?style=for-the-badge)
![Release](https://img.shields.io/badge/BETA-0.1.0--beta.2-2563EB?style=for-the-badge)
![GPU](https://img.shields.io/badge/GPU-MALI--G720-7C3AED?style=for-the-badge)
![API](https://img.shields.io/badge/API-VULKAN-E53935?style=for-the-badge)
![Android](https://img.shields.io/badge/PLATFORM-ANDROID-3DDC84?style=for-the-badge)

<br>

**PanVK nativo • Kbase/CSF • Android • Mali-G720**

</div>

---

> [!WARNING]
> **Projeto experimental.** Testes podem causar travamentos de aplicações, GPU faults ou exigir reinicialização do dispositivo.
>
> Este projeto **não declara conformidade Vulkan** e não declara compatibilidade universal com toda Mali-G720, todos os kernels Kbase ou todos os jogos.

# 🚀 Estado atual

O **Drive G720 / PanVK** adapta o Mesa/PanVK para executar diretamente sobre a **Arm Mali-G720**, usando a interface **Kbase/CSF** disponível em kernels Android.

A release pública atual é a **PanVK G720 0.1.0 Beta 2**, publicada como GitHub **Pre-release** sob a tag SemVer **`0.1.0-beta.2`**. A linha de desenvolvimento permanece em `g720-development` e já contém a sincronização documental pós-release.

| | Estado |
|---|---|
| **Release pública atual** | `0.1.0-beta.2` ✅ |
| **Classe GitHub** | Pre-release |
| **Tag / source snapshot** | `f1d7bed571766c49e5dd464f92d1fda264612311` |
| **Branch de desenvolvimento atual** | `g720-development` · `ca163891e8d3…` |
| **Autoridade técnica do binário** | `980ac91de74d…` |
| **Geometry Shader** | exposto e validado no escopo dirigido ✅ |
| **Tessellation** | caminho direto validado no escopo dirigido ✅ |
| **GPU WAIT64 interno** | promovido para dependências PanVK elegíveis ✅ |
| **GPU principal de validação** | Mali-G720 MC8 |
| **Foco atual** | regressões, logs comunitários, compatibilidade e otimização causal |

> A tag da Beta 2 aponta para o snapshot de source/documentação usado na publicação.
> O binário distribuído é byte-exact ao artefato qualificado no commit técnico `980ac91…`.
> Consulte **[PROVENANCE](docs/PROVENANCE.md)** para o binding completo.

## ⬇️ Beta pública atual

<div align="center">

### [Baixar PanVK G720 0.1.0 Beta 2](https://github.com/wonderkast02/panvk-g720-kbase-csf/releases/tag/0.1.0-beta.2)

`PanVK-G720-0.1.0-beta.2.zip`

**SHA-256:** `fc1d69647c071ca3fe30ae2fb450e95c91c08e90779eb32c865fa384dff5aaca`

</div>

> A Beta 2 segue SemVer convencional (`0.1.0-beta.N`) e é uma **pre-release**.
> GitHub não permite marcar pre-releases como `Latest`; a autoridade de download é o link explícito acima e `docs/RELEASES.md`.

---

# ✅ Recursos validados

| Recurso | Estado |
|---|:---:|
| Kbase / CSF userspace | ✅ |
| Inicialização Vulkan | ✅ |
| Compute | ✅ |
| Graphics pipeline | ✅ |
| Render offscreen | ✅ |
| CPU readback | ✅ |
| Texture sampling | ✅ |
| Depth / Stencil / Blending | ✅ |
| MSAA + Resolve | ✅ |
| X11 WSI / Swapchain | ✅ |
| Tessellation | ✅ |
| Geometry Shader | ✅ |
| Tessellation + GS | ✅ |
| Transform Feedback + GS | ✅ |
| Layered GS | ✅ |
| GPU WAIT64 interno elegível | ✅ |
| Box64 / Wine Vulkan | 🧪 |
| DXVK / D3D11 | 🧪 |
| Conformidade Vulkan | ❌ |

**✅ Validado** · **🧪 Experimental** · **❌ Não declarado**

Os resultados são válidos para o escopo e hardware efetivamente testados. Consulte [VALIDATION](docs/VALIDATION.md) antes de transformar qualquer resultado em claim de suporte geral.

---

# 📱 Plataforma de referência

| Componente | Configuração |
|---|---|
| **SoC** | MediaTek MT6899 |
| **GPU** | Mali-G720 MC8 |
| **GPU ID** | `0xc8700010` |
| **Vendor ID** | `0x13b5` |
| **Kernel driver** | Kbase / CSF |
| **Device node** | `/dev/mali0` |
| **Sistema** | Android |
| **API mínima do pacote Beta 2** | 35 |

> Outras variantes da Mali-G720 continuam experimentais até receberem validação independente.

Detalhes do bring-up, UAPI, memória e sincronização CSF: **[KBASE_CSF](docs/KBASE_CSF.md)**.

---

# 🧠 Arquitetura

```text
┌─────────────────────────┐
│    Aplicação Vulkan     │
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│      Mesa / PanVK       │
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│     Backend Kbase       │
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│        CSF / KMD        │
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│      Mali-G720 GPU      │
└─────────────────────────┘
```

O caminho principal é **PanVK nativo sobre Kbase/CSF**.

**Winlator, Vortek, Wine, Box64, DXVK e VKD3D não fazem parte da arquitetura interna do driver.** São camadas externas de integração, compatibilidade e validação.

Para sincronização, payloads binários locais/internos PanVK elegíveis podem usar espera GPU `SYNC64`/WAIT64. `sync_file` importado, timeline wrappers, conjuntos mistos, submits apenas de espera e conjuntos acima do limite continuam no fallback CPU/KCPU.

Detalhes: **[Arquitetura](docs/ARCHITECTURE.md)**.

---

# 🎮 DXVK & Winlator

A compatibilidade com workloads Windows continua sendo uma frente ativa.

Ambientes usados em testes dirigidos:

**Winlator** · **Vortek** · **Wine** · **Box64** · **DXVK**

Isso **não representa compatibilidade universal com jogos**. Uma aplicação iniciar ou renderizar parcialmente não é suficiente para declarar suporte geral.

### Foco pós-Beta 2

- classificar e reproduzir logs da comunidade;
- separar driver, runtime, transporte e aplicação;
- fortalecer Winlator / Vortek;
- reavaliar DXVK com o contrato de features atual;
- ampliar regressões e CTS focado;
- corrigir renderização antes de otimizar FPS;
- otimizar somente gargalos comprovados.

Histórico detalhado de DXVK, Wine/Box64, BCn e wrappers: **[COMPATIBILITY](docs/COMPATIBILITY.md)**.

---

# 🧪 Validação & documentação

| Documento | Conteúdo |
|---|---|
| **[STATUS](docs/STATUS.md)** | Estado técnico e público atual |
| **[VALIDATION](docs/VALIDATION.md)** | Testes, escopo e evidências |
| **[PROVENANCE](docs/PROVENANCE.md)** | Source, binário, hashes e autoridade |
| **[RELEASES](docs/RELEASES.md)** | Releases públicas e artefatos |
| **[VERSIONING](docs/VERSIONING.md)** | SemVer e política de pre-release |
| **[ARCHITECTURE](docs/ARCHITECTURE.md)** | Arquitetura do projeto |
| **[KBASE_CSF](docs/KBASE_CSF.md)** | UAPI, hardware, memória e CSF |
| **[COMPATIBILITY](docs/COMPATIBILITY.md)** | Winlator, DXVK, Wine, Box64 e wrappers |
| **[ROADMAP](docs/ROADMAP.md)** | Próximas etapas |
| **[HISTORY](docs/HISTORY.md)** | Marcos técnicos |
| **[COMMUNITY_TESTING](docs/COMMUNITY_TESTING.md)** | Guia de testes da comunidade |
| **[GOVERNANCE](docs/GOVERNANCE.md)** | Regras de branch/release |

### Princípio de desenvolvimento

> **Runtime > suposição estática**
>
> **Evidência bruta > classificador**

Build concluído **não significa automaticamente runtime aprovado**. Falha de tooling, transporte ou instrumentação também não deve ser convertida automaticamente em falha do driver.

---

# 🗺️ Roadmap

### Concluído no escopo atual

- [x] Kbase / CSF bring-up
- [x] Inicialização Vulkan
- [x] Compute
- [x] Graphics
- [x] WSI / Swapchain
- [x] Tessellation dirigida
- [x] Geometry Shader dirigido
- [x] Consolidação pós-GS
- [x] GPU WAIT64 interno qualificado/promovido
- [x] Beta 2 qualificada e publicada

### Próxima fase

- [ ] absorver e classificar regressões da comunidade;
- [ ] corrigir bugs reproduzíveis de renderização/sincronização;
- [ ] ampliar Winlator / Vortek / DXVK;
- [ ] ampliar regressões e CTS focado;
- [ ] validar outros dispositivos Mali-G720;
- [ ] trabalhar desempenho após correctness;
- [ ] preparar futura `0.1.0-beta.3` apenas quando houver candidato público qualificado.

Roadmap detalhado: **[docs/ROADMAP.md](docs/ROADMAP.md)**.

---

# 🤝 Comunidade

Quer testar, reportar problemas ou contribuir?

**[🧪 Guia de testes](docs/COMMUNITY_TESTING.md)** · **[Contribuir](CONTRIBUTING.md)** · **[Segurança](SECURITY.md)** · **[Licenciamento](LICENSING.md)**

Ao reportar um problema, inclua sempre que possível:

`Dispositivo` · `GPU` · `Kernel/Kbase` · `Tag` · `SHA-256 do pacote` · `Runtime` · `DXVK/VKD3D` · `Logs`

Resultados completos e reproduzíveis ajudam muito mais do que apenas informar que algo “funcionou” ou “não funcionou”.

---

# ⚠️ Limitações

- não há declaração de conformidade Vulkan;
- não há garantia de suporte universal a toda variante Mali-G720;
- não há garantia de compatibilidade com todos os jogos;
- não há garantia de compatibilidade com todas as versões do DXVK;
- features presentes em drivers proprietários ou wrappers não são automaticamente consideradas suporte nativo;
- builds de desenvolvimento podem regredir;
- resultados obtidos em um dispositivo não devem ser generalizados automaticamente para todo hardware Mali-G720.

---

<div align="center">

# Drive G720 / PanVK

### **Vulkan aberto na Mali-G720**

`Mali-G720` • `PanVK` • `Kbase/CSF` • `Android`

<br>

**Hardware real • Depuração causal • Evidência reproduzível**

</div>

---

# 💙 Créditos & Agradecimentos

O **Drive G720 / PanVK** existe graças a uma ampla base de software livre, engenharia reversa, pesquisa aberta e testes comunitários.

## 🧩 Bases e contribuidores

**[Leegao](https://github.com/leegao)** — pelo `mesa-funnymdzz`, pelo `bionic-vulkan-wrapper` e por trabalho público utilizado como base ou referência durante a integração PanVK/Kbase.

**[funnymdzz](https://github.com/funnymdzz)** — por trabalho-base e referências utilizadas na evolução do caminho PanVK/Kbase.

**Icecream95 / Panfork** — pelo trabalho pioneiro no ecossistema Panfrost/Panfork, engenharia reversa de GPUs Mali e infraestrutura relacionada.

**Saikatsaha1996** — por referências e experimentação comunitária envolvendo GPUs Mali modernas, CSF e Panfrost/PanVK.

**wonderkast02 / Drive G720** — pela integração, pesquisa, desenvolvimento, qualificação e validação específica do caminho Mali-G720 / Kbase / CSF deste projeto.

## 🌐 Upstream

**Mesa 3D** · **Panfrost** · **PanVK** · **Panfork**

e seus mantenedores e contribuidores.

## 🔺 Arm & Vulkan

**Arm** — arquitetura Mali, Kbase, CSF e interfaces utilizadas pelo projeto.

**Khronos Group** — Vulkan, SPIR-V e infraestrutura de testes/conformidade do ecossistema Vulkan.

## 🎮 Compatibilidade & validação

**DXVK** · **Wine** · **Box64** · **Winlator** · **Vortek**

## 🛠️ Ferramentas & infraestrutura

**Termux** · **Termux:X11** · **Android / AOSP / Bionic** · **Android NDK** · **LLVM / Clang** · **Meson** · **Ninja** · **SPIRV-Tools** · **VK-GL-CTS**

## 🔬 Referências técnicas históricas

- `leegao/bionic-vulkan-wrapper`
- `funnymdzz/mali_fxxker`
- `Saikatsaha1996/mesa-Panfrost-G610`
- `yoshi3jp/android_kernel_samsung_a25ex_mt6835`
- `nangitagamer777-art/Panvk_Kmod`
- forks e builds experimentais de DXVK usados durante investigação

A presença nesta lista **não significa necessariamente que código desses projetos esteja presente na build atual**.

## 🧑‍💻 Testadores & comunidade

Agradecimento a todos que contribuíram com testes em hardware Mali, logs, dumps, reproduções, descoberta de regressões, comparação entre ambientes, feedback, documentação, pesquisa e discussão técnica.

> **Cada projeto, arquivo e componente mantém seus próprios autores, copyrights e termos de licença.**
>
> A presença nesta seção não implica afiliação, patrocínio ou endosso oficial ao Drive G720.
