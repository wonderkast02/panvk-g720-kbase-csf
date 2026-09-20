# Proveniência de fonte e artefatos

A proveniência pública e a linha de desenvolvimento pós-release são mantidas separadas.

## Release pública `0.1.0-beta.1.9.4`

### Cadeia de fonte congelada

`ci 0521a3257628e811cfead6b5a9753e9f705e2f31`
→ staging Android/Bionic
→ deltas MC8 validados
→ composição FullPlane
→ Kbase dma-heap device-node `O_RDONLY`.

A release pública está vinculada a:

- branch: `android-candidate-beta-1.9.4`
- source commit: `3549264275c9663ed73e01d652f4c0d16f21df22`
- tag: `refs/tags/0.1.0-beta.1.9.4`
- package SHA-256: `01c6304206c6e348cb069e3d04fb1c7b693195b543b4134ad7c108a33906d1fa`
- embedded SO SHA-256: `05f867332924aacd91e6182cc1cc572ff04689cbcebeeba0e70bef61698dc9de`
- `meta.json` SHA-256: `01ef6b466751a5cb375073319ed70f872763ab71858b767c24d4ae9e737dae90`
- GNU Build ID: `4bc1dcd6ade70537a80e64bfc4976cb5936bf2af`

Esses valores não são modificados por desenvolvimento posterior.

### Estado Bionic preservado

- `src/panfrost/vulkan/panvk_device.h`: `dedd6ed1c96b46829c1d52c395d79608528dc8ff120640dc64ae116f6c810038`
- `src/util/u_gralloc/meson.build`: `da7bd6bcc7c2695284b4ec349bc259028a68a755ac0f7e1ae2f7b74efc463a43`

### Fingerprints MC8 históricos

- semantic fingerprint: `ad3f9fceb4ccd2b45e39a3893288fcf18cbc0b87c8aa98a827b05b88a67123ca`
- modified-path fingerprint: `97654e435d7e7662bde9e16692275dd5715d2455441de2cd9bbe2d1738afedcb`

### Composição final histórica da beta

- `src/util/u_gralloc/u_gralloc_fallback.c`: `7cf289004480183564a34b1f2e80690c32a0c42be74cb0b9b81ba07aff7d9fdc`
- `src/panfrost/lib/kmod/kbase_kmod.c`: `140b334c29a8999816e0084ce8b534b2171128a1da3de078e7b7509bf7431fe7`

A semântica Kbase é intencionalmente estreita: o **device node** do dma-heap é aberto `O_RDONLY | O_CLOEXEC`; a alocação DMA-HEAP continua retornando dma-buf FDs com `O_RDWR | O_CLOEXEC`.

`DRM_FORMAT_MOD_INVALID -> DRM_FORMAT_MOD_LINEAR` por suposição continua proibido.

## Linha de desenvolvimento pós-beta

O desenvolvimento posterior à beta avançou para Tessellation e Geometry Shader mais completos e concluiu o closeout funcional de GS no hardware de referência.

Essa linha **não possui vínculo público de release novo neste documento**.

Até que um novo candidato seja explicitamente escolhido:

- não atribuir nova beta;
- não criar tag por conveniência;
- não reutilizar a identidade da beta atual para bytes diferentes;
- não substituir hashes publicados;
- preservar a autoridade de fonte/build/runtime separadamente;
- registrar hashes e source binding antes de qualquer release futura.

## Build reproduzível

O repositório Git sozinho não é tratado como imagem hermética de build. Reproduzir os mesmos bytes também depende do ambiente externo documentado: Android NDK/API, Meson, toolchain e dependências locais pertinentes.

## Licenciamento

Proveniência técnica não substitui copyright ou licença. Consulte `LICENSING.md` e os SPDX/copyrights de cada arquivo.
