# PanVK G720 — Estado atual

Atualizado em: **2026-09-24**

## Resumo

O estado público atual é:

- **release pública:** `0.1.0-beta.2`;
- **título:** `PanVK G720 0.1.0 Beta 2`;
- **classe GitHub:** Pre-release;
- **tag / source snapshot:** `f1d7bed571766c49e5dd464f92d1fda264612311`;
- **branch de desenvolvimento:** `g720-development` em `ca163891e8d3367c4b65ecaf7dcb7452545f4172`;
- **autoridade técnica do binário:** `980ac91de74df5e5807e6269fd2531fa3ee6b4e5`.

A Beta 2 adota a sequência SemVer convencional `0.1.0-beta.N`.

## Layout do repositório

- `main`: landing pública e documentação ativa;
- `g720-development`: source authority de desenvolvimento e integração;
- `ci`: checkpoint histórico full-Mesa `0521a3257628e811cfead6b5a9753e9f705e2f31`;
- `android-candidate-beta-1.9.4`: lineage histórica congelada da Beta 1.9.4.

`main` e `g720-development` têm funções diferentes e não devem ser fundidos apenas para estética do grafo.

## Beta 2

Release ID GitHub: `396132659`

### Source

- tag: `0.1.0-beta.2`
- tag target: `f1d7bed571766c49e5dd464f92d1fda264612311`
- tree do tag target: `6e36b7712fdf71c28d73092c68370dbfd4b95bc8`
- commit técnico que produziu o binário qualificado: `980ac91de74df5e5807e6269fd2531fa3ee6b4e5`
- tree técnico: `06d46e5ff29783c47740ab2da0d6f54db4fcff21`

### Artefatos

- pacote: `PanVK-G720-0.1.0-beta.2.zip`
- package SHA-256: `fc1d69647c071ca3fe30ae2fb450e95c91c08e90779eb32c865fa384dff5aaca`
- embedded `libvulkan_panfrost.so` SHA-256: `126b8b6124a8677298469f520cd6c825de88b498fe93a38bd358ef3b557882d3`
- embedded SO size: `21,497,392` bytes
- `meta.json` SHA-256: `3b6590d07bd082c2b4600dbf264d886f55b42bffe6140bac5ab11ea748bd82ae`
- manifest SHA-256: `82d1436ef6873a184c8ac2f142d7b48e0437763fec52c32107f37939b7b0606b`
- `SHA256SUMS.txt` SHA-256: `6eab6525234519d8c5ba6bb99a54222272e0e9e2328754fd9b758b26adad4f7e`
- Android API mínimo: `35`

## Recursos relevantes da Beta 2

No escopo qualificado:

- Geometry Shader exposto;
- Tessellation exposta;
- direct tessellation com validação dirigida de hardware/semântica;
- GPU WAIT64 para dependências binárias locais/internas elegíveis;
- fallback CPU/KCPU preservado para `sync_file` importado, timeline wrappers, conjuntos mistos, wait-only e conjuntos acima do limite.

Isso não é claim de conformidade Vulkan.

## Desenvolvimento após a tag

Após o snapshot da tag, `g720-development` recebeu apenas a sincronização documental pós-release:

- commit: `ca163891e8d3367c4b65ecaf7dcb7452545f4172`
- tree: `70a09e9f88f5fe7ed595f3bf3347f5b32173daf9`

A autoridade técnica do binário distribuído permanece `980ac91...`.

## Beta 1.9.4 histórica

A tag `0.1.0-beta.1.9.4` e seus assets permanecem imutáveis como registro histórico. Não reutilizar essa identidade para bytes diferentes.

## Foco atual

- regressões e logs da comunidade;
- bugs reproduzíveis de renderização/sincronização;
- Winlator / Vortek / DXVK;
- CTS focado e regressões;
- performance somente após correctness.

## Limite de claims

O projeto não declara:

- conformidade Vulkan;
- compatibilidade universal com Mali-G720;
- compatibilidade universal com jogos;
- que resultados do MC8 representam toda variante G720;
- que uma feature presente em driver proprietário existe automaticamente no PanVK.

Consulte também [VALIDATION.md](VALIDATION.md), [PROVENANCE.md](PROVENANCE.md), [RELEASES.md](RELEASES.md) e [VERSIONING.md](VERSIONING.md).
