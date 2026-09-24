# Governança do repositório

## Função das branches

- `main`: landing pública, políticas e documentação pública;
- `g720-development`: source authority ativa de desenvolvimento;
- `ci`: checkpoint histórico full-Mesa preservado;
- `android-candidate-beta-1.9.4`: lineage histórica congelada da Beta 1.9.4.

Branches congeladas não recebem manutenção comum de documentação ou refactors.

## Estado autoritativo atual

- release pública: `0.1.0-beta.2`
- tag target: `f1d7bed571766c49e5dd464f92d1fda264612311`
- branch de desenvolvimento: `ca163891e8d3367c4b65ecaf7dcb7452545f4172`
- autoridade técnica do binário Beta 2: `980ac91de74df5e5807e6269fd2531fa3ee6b4e5`

O commit pós-tag `ca163891...` é documental e não redefine os bytes do binário Beta 2.

## Separação entre desenvolvimento e release

O projeto diferencia:

1. desenvolvimento técnico;
2. consolidação Git;
3. qualificação de release;
4. publicação pública;
5. sincronização documental pós-release.

Fechar uma feature, consolidar código ou atualizar documentação não implica automaticamente nova versão.

## Versionamento

A partir da Beta 2:

- beta pública: `0.1.0-beta.N`
- release candidate: `0.1.0-rc.N`
- stable: `0.1.0`

Beta/RC são GitHub pre-releases.

Tags históricas não são renomeadas.

## Integridade de releases

A Beta 2 é vinculada a:

- tag `0.1.0-beta.2`;
- source snapshot `f1d7bed571766c49e5dd464f92d1fda264612311`;
- package SHA-256 `fc1d69647c071ca3fe30ae2fb450e95c91c08e90779eb32c865fa384dff5aaca`;
- embedded SO SHA-256 `126b8b6124a8677298469f520cd6c825de88b498fe93a38bd358ef3b557882d3`.

Tags e artefatos publicados não são substituídos silenciosamente.

## Evidência

Princípios de qualificação:

- runtime > grep;
- evidência bruta > classificador;
- compile > static guess quando a compilação é o teste correto;
- first real fail > ruído posterior;
- falha de instrumentação != falha do driver;
- transporte indisponível != falha do driver;
- archive integrity pass != technical pass;
- correctness > FPS.

Features publicamente anunciadas devem ter implementação e validação compatíveis com o claim.

## Histórico Git

Não reescrever histórico apenas para estética. Históricos distintos preservados por motivo técnico/proveniência não devem ser fundidos à força.

## Segurança

Relatórios sensíveis seguem `SECURITY.md`. Testes comunitários seguem `docs/COMMUNITY_TESTING.md`.

## Licenciamento

Esta governança não substitui nem normaliza licenças. Consulte `LICENSING.md` e os identificadores SPDX/copyrights por arquivo.
