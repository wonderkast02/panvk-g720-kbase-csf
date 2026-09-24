# Contribuindo com o PanVK G720

Obrigado por ajudar a testar, documentar ou melhorar o Drive G720 / PanVK.

## Antes de contribuir

Leia:

- `README.md`
- `docs/STATUS.md`
- `docs/VALIDATION.md`
- `docs/PROVENANCE.md`
- `docs/VERSIONING.md`
- `LICENSING.md`

## Layout do repositório

- `main`: documentação pública e landing ativa;
- `g720-development`: branch ativa de source/desenvolvimento;
- `ci`: checkpoint histórico full-Mesa;
- `android-candidate-beta-1.9.4`: source lineage histórica congelada.

Release pública atual: `0.1.0-beta.2`.

Não envie mudanças comuns para branches históricas congeladas.

## Bug reports

Inclua:

- dispositivo, SoC e GPU;
- Android;
- kernel/Kbase;
- tag/build exato;
- SHA-256 do pacote;
- runtime;
- DXVK/VKD3D quando aplicável;
- aplicação/jogo;
- passos de reprodução;
- logs com timestamps.

Preserve o pacote original quando o resultado pretende descrever uma release oficial.

## Mudanças de código

Uma alteração útil deve:

1. identificar o problema causal;
2. manter comportamento Kbase/CSF na camada apropriada;
3. evitar feature advertisement sem implementação e validação;
4. incluir evidência de build/runtime compatível com o risco da mudança;
5. preservar proveniência e versionamento;
6. evitar refactors não relacionados durante correções causais;
7. nunca converter `DRM_FORMAT_MOD_INVALID` em `DRM_FORMAT_MOD_LINEAR` por suposição;
8. priorizar correctness antes de performance.

## Mudanças de documentação

Documentação deve separar claramente:

- estado da release pública;
- estado de desenvolvimento;
- evidência;
- inferência;
- limitações.

Não transforme resultado interno em claim público mais amplo.

## Claims

Não descreva o projeto como Vulkan conformant, universalmente compatível com Mali-G720 ou compatível com todos os jogos sem evidência independente suficiente.

## Segurança

Não publique credenciais, tokens, dados privados de dispositivo ou detalhes de vulnerabilidade sensível em issue pública. Consulte `SECURITY.md`.
