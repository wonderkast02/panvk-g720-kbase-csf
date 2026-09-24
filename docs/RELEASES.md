# Releases

## `0.1.0-beta.2` — Public Beta / GitHub Pre-release

Publicada em: **2026-09-24**

Release:
https://github.com/wonderkast02/panvk-g720-kbase-csf/releases/tag/0.1.0-beta.2

### Identidade

- title: `PanVK G720 0.1.0 Beta 2`
- tag: `0.1.0-beta.2`
- tag target: `f1d7bed571766c49e5dd464f92d1fda264612311`
- release ID: `396132659`
- prerelease: `true`
- immutable: `true`
- technical binary authority: `980ac91de74df5e5807e6269fd2531fa3ee6b4e5`

### Artefatos

- `PanVK-G720-0.1.0-beta.2.zip`
  - SHA-256: `fc1d69647c071ca3fe30ae2fb450e95c91c08e90779eb32c865fa384dff5aaca`
- `PanVK-G720-0.1.0-beta.2-MANIFEST.txt`
  - SHA-256: `82d1436ef6873a184c8ac2f142d7b48e0437763fec52c32107f37939b7b0606b`
- `SHA256SUMS.txt`
  - SHA-256: `6eab6525234519d8c5ba6bb99a54222272e0e9e2328754fd9b758b26adad4f7e`
- embedded `libvulkan_panfrost.so`
  - SHA-256: `126b8b6124a8677298469f520cd6c825de88b498fe93a38bd358ef3b557882d3`
- embedded `meta.json`
  - SHA-256: `3b6590d07bd082c2b4600dbf264d886f55b42bffe6140bac5ab11ea748bd82ae`

### Escopo

A Beta 2 consolida o checkpoint pós-GS e inclui o caminho interno GPU WAIT64 para dependências PanVK elegíveis.

Não é:

- release estável;
- claim de conformidade Vulkan;
- claim de compatibilidade universal com Mali-G720;
- garantia de compatibilidade com jogos.

## `0.1.0-beta.1.9.4` — Historical Public Beta

Publicada em: **2026-09-01**

Release:
https://github.com/wonderkast02/panvk-g720-kbase-csf/releases/tag/0.1.0-beta.1.9.4

### Identidade histórica preservada

- tag: `0.1.0-beta.1.9.4`
- source branch: `android-candidate-beta-1.9.4`
- source commit: `3549264275c9663ed73e01d652f4c0d16f21df22`
- package SHA-256: `01c6304206c6e348cb069e3d04fb1c7b693195b543b4134ad7c108a33906d1fa`
- embedded SO SHA-256: `05f867332924aacd91e6182cc1cc572ff04689cbcebeeba0e70bef61698dc9de`

A identidade histórica não é renomeada nem reescrita.

## Política GitHub

Betas e release candidates são GitHub **Pre-releases**.

GitHub não permite que pre-releases sejam marcadas como `Latest`. Portanto, enquanto o projeto estiver apenas em beta/RC, o badge `Latest` não deve ser usado como autoridade para identificar a beta pública atual.

A autoridade é:

1. `README.md`;
2. este documento;
3. a tag explícita da release;
4. hashes e manifest publicados.

## Próxima versão

A próxima beta pública, se houver candidato qualificado e decisão explícita de lançamento, será `0.1.0-beta.3`.
