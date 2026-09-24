# Proveniência de fonte e artefatos

A proveniência pública separa claramente:

1. snapshot da tag;
2. autoridade técnica que produziu o binário;
3. estado atual da branch de desenvolvimento;
4. releases históricas.

## Beta 2 — release pública atual

### Git / source snapshot

- tag: `0.1.0-beta.2`
- tag target: `f1d7bed571766c49e5dd464f92d1fda264612311`
- tree: `6e36b7712fdf71c28d73092c68370dbfd4b95bc8`
- GitHub release ID: `396132659`

O commit `f1d7bed...` é o snapshot selecionado para a tag e contém o código técnico já qualificado mais documentação sincronizada.

### Autoridade técnica do binário

O ELF distribuído foi produzido e qualificado no commit:

- commit: `980ac91de74df5e5807e6269fd2531fa3ee6b4e5`
- tree: `06d46e5ff29783c47740ab2da0d6f54db4fcff21`
- commit subject: `panvk: integrate validated internal GPU WAIT64 waits`

O commit `f1d7bed...` é filho documental de `980ac91...`; não altera os source bytes que produziram o ELF.

### Artefatos Beta 2

- package: `PanVK-G720-0.1.0-beta.2.zip`
- package SHA-256: `fc1d69647c071ca3fe30ae2fb450e95c91c08e90779eb32c865fa384dff5aaca`
- embedded SO SHA-256: `126b8b6124a8677298469f520cd6c825de88b498fe93a38bd358ef3b557882d3`
- embedded SO size: `21,497,392`
- `meta.json` SHA-256: `3b6590d07bd082c2b4600dbf264d886f55b42bffe6140bac5ab11ea748bd82ae`
- manifest SHA-256: `82d1436ef6873a184c8ac2f142d7b48e0437763fec52c32107f37939b7b0606b`
- `SHA256SUMS.txt` SHA-256: `6eab6525234519d8c5ba6bb99a54222272e0e9e2328754fd9b758b26adad4f7e`

A qualificação final incluiu build alvo, full build, Meson test, auditoria ELF e roundtrip dos assets publicados.

## Desenvolvimento após a tag

A branch `g720-development` está atualmente em:

- commit: `ca163891e8d3367c4b65ecaf7dcb7452545f4172`
- tree: `70a09e9f88f5fe7ed595f3bf3347f5b32173daf9`

Esse commit é uma sincronização documental pós-release. A autoridade técnica do binário Beta 2 não muda.

## WAIT64 interno

O caminho promovido em `980ac91...` é deliberadamente estreito:

- payloads binários locais/internos PanVK elegíveis podem usar GPU `SYNC64`/WAIT64;
- condição validada: `GREATER(target - 1)`;
- até três wait cells por submit path;
- waits são emitidos antes da aquisição de recursos;
- `sync_file` importado, timeline wrappers, conjuntos mistos, wait-only e oversized permanecem no fallback CPU/KCPU.

## Beta 1.9.4 histórica

A release histórica continua vinculada a:

- branch: `android-candidate-beta-1.9.4`
- source commit: `3549264275c9663ed73e01d652f4c0d16f21df22`
- tag: `0.1.0-beta.1.9.4`
- package SHA-256: `01c6304206c6e348cb069e3d04fb1c7b693195b543b4134ad7c108a33906d1fa`
- embedded SO SHA-256: `05f867332924aacd91e6182cc1cc572ff04689cbcebeeba0e70bef61698dc9de`

Esses valores são históricos e não são modificados pela Beta 2.

## Build reproduzível

O repositório Git sozinho não é uma imagem hermética de build. Reproduzir os mesmos bytes também depende do ambiente externo documentado, incluindo Android NDK/API, Meson, toolchain e dependências locais pertinentes.

## Licenciamento

Proveniência técnica não substitui copyright ou licença. Consulte `LICENSING.md` e os SPDX/copyrights de cada arquivo.
