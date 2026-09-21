<div align="center">

# PanVK para Mali-G720

### Vulkan experimental sobre Kbase/CSF no Android

**`Vulkan → Mesa/PanVK → Kbase/CSF → Mali-G720`**

<br>

![Status](https://img.shields.io/badge/STATUS-EXPERIMENTAL-F59E0B?style=for-the-badge)
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
> Este projeto **não declara conformidade Vulkan**.

# 🚀 Estado atual

O **Drive G720 / PanVK** adapta o Mesa/PanVK para executar diretamente sobre a **Arm Mali-G720**, usando a interface **Kbase/CSF** disponível em kernels Android.

A linha atual de desenvolvimento concluiu o **fechamento funcional do Geometry Shader** e está consolidada na branch **`g720-development`**. Isso é posterior à beta pública atual e **não cria automaticamente uma nova release**.

| | Estado |
|---|---|
| **Linha de desenvolvimento** | Geometry Shader funcionalmente fechado ✅ |
| **Beta pública atual** | `0.1.0-beta.1.9.4` |
| **Branch de desenvolvimento** | `g720-development` · `77832026e87f…` |
| **GPU principal de validação** | Mali-G720 MC8 |
| **Foco atual** | Consolidação, compatibilidade, regressões e otimização |

## ⬇️ Beta pública

<div align="center">

### [Baixar PanVK G720 0.1.0-beta.1.9.4](https://github.com/wonderkast02/panvk-g720-kbase-csf/releases/tag/0.1.0-beta.1.9.4)

</div>

> A beta pública pode ficar atrás da linha atual de desenvolvimento.
>
> Uma nova beta só é publicada após qualificação específica e decisão explícita de release.

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
| Box64 / Wine Vulkan | 🧪 |
| DXVK / D3D11 | 🧪 |
| Conformidade Vulkan | ❌ |

**✅ Validado** · **🧪 Experimental** · **❌ Não declarado**

Os resultados são válidos para o escopo e hardware efetivamente testados. Consulte [VALIDATION](docs/VALIDATION.md) antes de transformar qualquer resultado em claim de suporte geral.

---

# 📱 Plataforma validada

A plataforma principal usada como referência autoritativa de desenvolvimento e validação é:

| Componente | Configuração |
|---|---|
| **SoC** | MediaTek MT6899 |
| **GPU** | Mali-G720 MC8 |
| **GPU ID** | `0xc8700010` |
| **Vendor ID** | `0x13b5` |
| **Kernel driver** | Kbase / CSF |
| **Device node** | `/dev/mali0` |
| **Sistema** | Android |

> Outras variantes da Mali-G720 continuam experimentais até receberem validação independente.

Detalhes do bring-up, UAPI e correções CSF: **[KBASE_CSF](docs/KBASE_CSF.md)**.

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

**Winlator, Vortek, Wine, Box64 e DXVK não fazem parte da arquitetura interna do driver.** Eles são usados como camadas de integração, compatibilidade e validação.

Detalhes: **[Arquitetura](docs/ARCHITECTURE.md)**.

---

# 🎮 DXVK & Winlator

A compatibilidade com workloads Windows continua sendo uma frente ativa.

Já existem testes controlados com:

**Winlator** · **Vortek** · **Wine** · **Box64** · **DXVK**

Isso **não representa compatibilidade universal com jogos**. Uma aplicação iniciar ou renderizar parcialmente não é suficiente para declarar suporte geral.

### Foco pós-GS

- consolidar a source tree;
- melhorar reprodutibilidade;
- fortalecer Winlator / Vortek;
- reavaliar DXVK;
- ampliar regressões e CTS focado;
- otimizar desempenho somente após estabilidade funcional.

Histórico detalhado de DXVK, Wine/Box64, BCn e wrappers: **[COMPATIBILITY](docs/COMPATIBILITY.md)**.

---

# 🧪 Validação & documentação

| Documento | Conteúdo |
|---|---|
| **[STATUS](docs/STATUS.md)** | Estado técnico atual |
| **[VALIDATION](docs/VALIDATION.md)** | Testes, escopo e evidências |
| **[PROVENANCE](docs/PROVENANCE.md)** | Proveniência e autoridade |
| **[ARCHITECTURE](docs/ARCHITECTURE.md)** | Arquitetura do projeto |
| **[KBASE_CSF](docs/KBASE_CSF.md)** | UAPI, hardware e bring-up Kbase/CSF |
| **[COMPATIBILITY](docs/COMPATIBILITY.md)** | Winlator, DXVK, Wine, Box64 e wrappers |
| **[ROADMAP](docs/ROADMAP.md)** | Próximas etapas |
| **[HISTORY](docs/HISTORY.md)** | Marcos técnicos |
| **[RELEASES](docs/RELEASES.md)** | Releases públicas |
| **[VERSIONING](docs/VERSIONING.md)** | Política de versionamento |

### Princípio de desenvolvimento

> **Runtime > suposição estática**
>
> **Evidência bruta > classificador**

Build concluído **não significa automaticamente runtime aprovado**. Falha de tooling, transporte ou instrumentação também não deve ser convertida automaticamente em falha do driver.

---

# 🗺️ Roadmap

### Base gráfica

- [x] Kbase / CSF bring-up
- [x] Inicialização Vulkan
- [x] Compute
- [x] Graphics
- [x] WSI / Swapchain
- [x] Tessellation
- [x] Geometry Shader

### Próxima fase

- [ ] Consolidar a árvore pós-GS
- [ ] Reforçar reprodutibilidade de builds
- [ ] Ampliar Winlator / Vortek
- [ ] Reavaliar DXVK
- [ ] Expandir regressões e CTS focado
- [ ] Validar outros dispositivos Mali-G720
- [ ] Trabalhar desempenho e otimizações

Roadmap detalhado: **[docs/ROADMAP.md](docs/ROADMAP.md)**.

---

# 🤝 Comunidade

Quer testar, reportar problemas ou contribuir?

**[🧪 Guia de testes](docs/COMMUNITY_TESTING.md)** · **[Contribuir](CONTRIBUTING.md)** · **[Segurança](SECURITY.md)** · **[Licenciamento](LICENSING.md)**

Ao reportar um problema, inclua sempre que possível:

`Dispositivo` · `GPU` · `Kernel/Kbase` · `Driver` · `Runtime` · `Logs`

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

Agradecimento aos projetos e comunidades:

**Mesa 3D** · **Panfrost** · **PanVK** · **Panfork**

e aos seus mantenedores e contribuidores.

## 🔺 Arm & Vulkan

**Arm** — arquitetura Mali, Kbase, CSF e interfaces utilizadas pelo projeto.

**Khronos Group** — Vulkan, SPIR-V e infraestrutura de testes/conformidade do ecossistema Vulkan.

## 🎮 Compatibilidade & validação

Projetos usados em diferentes etapas de integração, diagnóstico ou validação:

**DXVK** · **Wine** · **Box64** · **Winlator** · **Vortek**

## 🛠️ Ferramentas & infraestrutura

**Termux** · **Termux:X11** · **Android / AOSP / Bionic** · **Android NDK** · **LLVM / Clang** · **Meson** · **Ninja** · **SPIRV-Tools** · **VK-GL-CTS**

## 🔬 Referências técnicas históricas

Entre os projetos e forks consultados ou usados como referência ao longo da pesquisa estão:

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
