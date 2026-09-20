# Releases

## `0.1.0-beta.1.9.4` — Public Beta / Pre-release

Publicada em: **2026-09-01**

Release:
https://github.com/wonderkast02/panvk-g720-kbase-csf/releases/tag/0.1.0-beta.1.9.4

### Identidade

- tag: `0.1.0-beta.1.9.4`
- source branch: `android-candidate-beta-1.9.4`
- source commit: `3549264275c9663ed73e01d652f4c0d16f21df22`
- base histórica `ci`: `0521a3257628e811cfead6b5a9753e9f705e2f31`
- pacote: `PanVK-G720-0.1.0-beta.1.9.4.zip`
- package SHA-256: `01c6304206c6e348cb069e3d04fb1c7b693195b543b4134ad7c108a33906d1fa`
- embedded SO SHA-256: `05f867332924aacd91e6182cc1cc572ff04689cbcebeeba0e70bef61698dc9de`
- `meta.json` SHA-256: `01ef6b466751a5cb375073319ed70f872763ab71858b767c24d4ae9e737dae90`
- GNU Build ID: `4bc1dcd6ade70537a80e64bfc4976cb5936bf2af`
- Android API mínimo: `35`

### Escopo

É uma beta comunitária / GitHub Pre-release.

Não é:

- release estável;
- claim de conformidade Vulkan;
- claim de compatibilidade universal com Mali-G720;
- garantia de compatibilidade com jogos.

A linha autoritativa MC8 já possuía tessellation dirigido/CTS na publicação. O run Winlator/Vortek congelado para a beta chegou ao primeiro `vkQueueSubmit`, mas não chegou a acquire/present.

## Desenvolvimento após `0.1.0-beta.1.9.4`

O projeto continuou evoluindo após a publicação e concluiu o fechamento funcional dirigido de Geometry Shader no hardware de referência.

Isso **não criou uma nova release, tag ou número de beta**.

Uma futura release só será adicionada aqui quando:

1. um candidato público for explicitamente escolhido;
2. source e package bytes estiverem congelados;
3. hashes e proveniência forem registrados;
4. qualificação de release estiver concluída;
5. a publicação for autorizada explicitamente.
