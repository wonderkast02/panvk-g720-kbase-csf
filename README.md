<div align="center">

# PanVK para Mali-G720

### Vulkan experimental sobre Kbase/CSF no Android

**`Vulkan → Mesa/PanVK → Kbase/CSF → Mali-G720`**

<br>

![Estado](https://img.shields.io/badge/ESTADO-BETA_PÚBLICA-F59E0B?style=for-the-badge)
![Versão](https://img.shields.io/badge/VERSÃO-0.1.0--beta.2-2563EB?style=for-the-badge)
![GPU](https://img.shields.io/badge/GPU-MALI--G720-7C3AED?style=for-the-badge)
![API](https://img.shields.io/badge/API-VULKAN-E53935?style=for-the-badge)
![Plataforma](https://img.shields.io/badge/PLATAFORMA-ANDROID-3DDC84?style=for-the-badge)

<br>

**PanVK nativo • Kbase/CSF • Android • Mali-G720**

</div>

---

> [!WARNING]
> **Projeto experimental.** Testes podem travar aplicações, causar falhas da GPU ou exigir reinicialização do dispositivo.
>
> O projeto **não declara conformidade Vulkan** nem compatibilidade universal.

# 🚀 Estado atual

O **Drive G720 / PanVK** adapta o Mesa/PanVK para executar diretamente na **Arm Mali-G720** por meio de **Kbase/CSF** em Android.

| | Estado |
|---|---|
| **Beta pública atual** | `0.1.0-beta.2` ✅ |
| **Desenvolvimento** | `g720-development` |
| **Geometry Shader** | validado no escopo dirigido ✅ |
| **Tessellation** | validada no escopo dirigido ✅ |
| **GPU WAIT64 interno** | habilitado para dependências elegíveis ✅ |
| **GPU principal de validação** | Mali-G720 MC8 |
| **Foco atual** | regressões, compatibilidade e otimização causal |

## ⬇️ Beta pública atual

<div align="center">

### [Baixar PanVK G720 0.1.0 Beta 2](https://github.com/wonderkast02/panvk-g720-kbase-csf/releases/tag/0.1.0-beta.2)

`PanVK-G720-0.1.0-beta.2.zip`

**SHA-256:** `fc1d69647c071ca3fe30ae2fb450e95c91c08e90779eb32c865fa384dff5aaca`

</div>

A proveniência completa de fonte, binário e hashes está em **[PROVENANCE](docs/PROVENANCE.md)**.

---

# ✅ Recursos validados

| Recurso | Estado |
|---|:---:|
| Kbase / CSF em espaço de usuário | ✅ |
| Inicialização Vulkan | ✅ |
| Computação | ✅ |
| Pipeline gráfico | ✅ |
| Renderização fora da tela | ✅ |
| Leitura pela CPU | ✅ |
| Amostragem de texturas | ✅ |
| Profundidade / Estêncil / Mistura | ✅ |
| MSAA + resolução | ✅ |
| X11 WSI / cadeia de apresentação | ✅ |
| Tessellation | ✅ |
| Geometry Shader | ✅ |
| Tessellation + GS | ✅ |
| Transform Feedback + GS | ✅ |
| GS em múltiplas camadas | ✅ |
| GPU WAIT64 interno elegível | ✅ |
| Box64 / Wine com Vulkan | 🧪 |
| DXVK / D3D11 | 🧪 |
| Conformidade Vulkan | ❌ |

**✅ Validado** · **🧪 Experimental** · **❌ Não declarado**

---

# 📱 Plataforma de referência

| Componente | Configuração |
|---|---|
| **SoC** | MediaTek MT6899 |
| **GPU** | Mali-G720 MC8 |
| **ID da GPU** | `0xc8700010` |
| **ID do fabricante** | `0x13b5` |
| **Driver do kernel** | Kbase / CSF |
| **Dispositivo** | `/dev/mali0` |
| **Sistema** | Android |
| **API mínima da Beta 2** | 35 |

> Outras variantes da Mali-G720 permanecem experimentais até validação independente.

Detalhes: **[KBASE_CSF](docs/KBASE_CSF.md)**.

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

O caminho principal é **PanVK nativo sobre Kbase/CSF**. Winlator, Vortek, Wine, Box64, DXVK e VKD3D são camadas externas de compatibilidade e teste.

Detalhes: **[Arquitetura](docs/ARCHITECTURE.md)**.

---

# 🎮 Compatibilidade com DXVK e Winlator

A compatibilidade com aplicações Windows continua experimental.

Prioridades atuais:

- reproduzir e corrigir regressões;
- separar falhas do driver, ambiente e aplicação;
- ampliar testes com Winlator, Vortek e DXVK;
- corrigir renderização antes de otimizar desempenho.

Detalhes: **[Compatibilidade](docs/COMPATIBILITY.md)**.

---

# 🧪 Validação e documentação

| Documento | Conteúdo |
|---|---|
| **[Estado](docs/STATUS.md)** | Estado técnico e público |
| **[Validação](docs/VALIDATION.md)** | Testes e evidências |
| **[Proveniência](docs/PROVENANCE.md)** | Fonte, binário e hashes |
| **[Lançamentos](docs/RELEASES.md)** | Releases e artefatos |
| **[Versionamento](docs/VERSIONING.md)** | Política SemVer |
| **[Arquitetura](docs/ARCHITECTURE.md)** | Arquitetura do projeto |
| **[Kbase/CSF](docs/KBASE_CSF.md)** | UAPI, memória e CSF |
| **[Compatibilidade](docs/COMPATIBILITY.md)** | Winlator, DXVK, Wine e Box64 |
| **[Próximas etapas](docs/ROADMAP.md)** | Direção técnica |
| **[Histórico](docs/HISTORY.md)** | Marcos técnicos |
| **[Testes da comunidade](docs/COMMUNITY_TESTING.md)** | Como testar e relatar |
| **[Governança](docs/GOVERNANCE.md)** | Regras do repositório |

---

# 🗺️ Próximas etapas

### Concluído no escopo atual

- [x] Kbase / CSF
- [x] Vulkan, computação e gráficos
- [x] WSI / cadeia de apresentação
- [x] Tessellation
- [x] Geometry Shader
- [x] GPU WAIT64 interno
- [x] Beta 2 publicada

### Próxima fase

- [ ] corrigir regressões reproduzíveis;
- [ ] ampliar testes com Winlator / Vortek / DXVK;
- [ ] ampliar regressões e CTS focado;
- [ ] validar outras variantes da Mali-G720;
- [ ] otimizar gargalos comprovados;
- [ ] preparar nova beta somente quando houver candidato qualificado.

---

# 🤝 Comunidade

**[🧪 Guia de testes](docs/COMMUNITY_TESTING.md)** · **[Contribuir](CONTRIBUTING.md)** · **[Segurança](SECURITY.md)** · **[Licenciamento](LICENSING.md)**

Ao relatar um problema, inclua dispositivo, GPU, Kbase, tag, SHA-256 do pacote, ambiente de execução, DXVK/VKD3D e logs relevantes.

---

# ⚠️ Limitações

- sem declaração de conformidade Vulkan;
- sem garantia de compatibilidade universal com Mali-G720;
- sem garantia de compatibilidade com todos os jogos ou versões do DXVK;
- recursos de drivers proprietários ou wrappers não implicam suporte nativo;
- builds de desenvolvimento podem apresentar regressões.

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
