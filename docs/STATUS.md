# PanVK G720 — Estado atual

Atualizado em: **2026-09-20**

## Resumo

O projeto mantém duas realidades deliberadamente separadas:

- **release pública atual:** `0.1.0-beta.1.9.4`;
- **linha de desenvolvimento pós-beta:** Geometry Shader funcionalmente fechado em hardware de referência, seguida por consolidação de source tree, regressões e compatibilidade.

O fechamento pós-beta **não altera os bytes, a tag ou a identidade da release pública existente** e não autoriza por si só uma nova beta.

## Layout do repositório

- `main`: landing pública e documentação;
- `ci`: checkpoint histórico full-Mesa `0521a3257628e811cfead6b5a9753e9f705e2f31`;
- `android-candidate-beta-1.9.4`: fonte congelada da release pública `0.1.0-beta.1.9.4`.

`main` e `ci` preservam históricos distintos; não devem ser mesclados ou reescritos apenas para “normalizar” o grafo.

## Release pública

A release pública atual permanece **`0.1.0-beta.1.9.4`**, publicada como **GitHub Pre-release / Public Beta**.

- source commit: `3549264275c9663ed73e01d652f4c0d16f21df22`
- pacote: `PanVK-G720-0.1.0-beta.1.9.4.zip`
- ZIP SHA-256: `01c6304206c6e348cb069e3d04fb1c7b693195b543b4134ad7c108a33906d1fa`
- SO limpo SHA-256: `05f867332924aacd91e6182cc1cc572ff04689cbcebeeba0e70bef61698dc9de`
- raw candidate SO SHA-256: `54a3a7dc9c972cc364058846e9b7ede57d4ac32d0902e8c8288e8fe89b9bb9bb`
- `meta.json` SHA-256: `01ef6b466751a5cb375073319ed70f872763ab71858b767c24d4ae9e737dae90`
- GNU Build ID: `4bc1dcd6ade70537a80e64bfc4976cb5936bf2af`
- API Android mínimo: `35`

Esses identificadores permanecem imutáveis para essa release.

## Linha de desenvolvimento pós-beta

Desde a publicação da beta, a linha de desenvolvimento avançou além do estado documentado em 2026-09-01.

O escopo dirigido de Geometry Shader foi fechado funcionalmente no hardware de referência, incluindo combinações relevantes com tessellation, transform feedback e layered rendering. Essa linha permanece **desenvolvimento**, não uma release pública nova.

Estado operacional atual:

- Geometry Shader: **fechamento funcional concluído**;
- source tree pós-GS: **em consolidação/auditoria**;
- nova beta: **não atribuída**;
- nova tag: **não criada**;
- release pública nova: **não autorizada**;
- otimização: posterior à estabilização funcional e às regressões necessárias.

## Limite de claims

O projeto não declara:

- conformidade Vulkan;
- compatibilidade universal com Mali-G720;
- compatibilidade universal com jogos;
- que resultados do MC8 representam toda variante G720;
- que uma feature presente em driver proprietário existe automaticamente no PanVK.

Consulte também [VALIDATION.md](VALIDATION.md), [PROVENANCE.md](PROVENANCE.md) e [VERSIONING.md](VERSIONING.md).
