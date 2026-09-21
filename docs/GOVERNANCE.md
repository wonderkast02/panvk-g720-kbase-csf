# Governança do repositório

## Função das branches

- `main`: landing pública e documentação ativa;
- `g720-development`: source authority de desenvolvimento pós-GS;
- `ci`: checkpoint histórico full-Mesa preservado;
- `android-candidate-beta-1.9.4`: lineage congelada da release `0.1.0-beta.1.9.4`.

Branches congeladas não devem receber manutenção comum de documentação ou refactors.

## Separação entre desenvolvimento e release

O projeto diferencia:

1. desenvolvimento técnico;
2. consolidação Git;
3. qualificação de release;
4. publicação pública.

Fechar uma feature, consolidar código ou atualizar documentação não implica automaticamente nova versão.

## Proteção da branch de desenvolvimento

`g720-development` preserva a autoridade pós-GS. No estado atual:

- force-push: bloqueado;
- deleção: bloqueada;
- histórico linear: exigido;
- enforcement para administradores: ativo;
- status `Drive-G720/PPA6-audit`: registra a qualificação externa do commit autoritativo.

O status PPA6 não deve ser apresentado como GitHub Actions/CI interno; ele referencia uma qualificação externa já concluída.

## Integridade de releases

A beta pública atual é vinculada a:

- tag `0.1.0-beta.1.9.4`;
- source commit `3549264275c9663ed73e01d652f4c0d16f21df22`;
- package SHA-256 `01c6304206c6e348cb069e3d04fb1c7b693195b543b4134ad7c108a33906d1fa`.

Tags e artefatos publicados não devem ser substituídos silenciosamente.

## Evidência

Princípios de qualificação:

- runtime > grep;
- evidência bruta > classificador;
- compile > static guess quando a compilação é o teste correto;
- first real fail > ruído posterior;
- falha de instrumentação != falha do driver;
- transporte indisponível != falha do driver;
- archive integrity pass != technical pass.

Features publicamente anunciadas devem ter implementação e validação compatíveis com o claim.

## Histórico Git

Não reescrever histórico apenas para estética. Históricos distintos preservados por motivo técnico/proveniência não devem ser fundidos à força.

## Segurança

Relatórios sensíveis devem seguir `SECURITY.md`. Testes comunitários seguem `docs/COMMUNITY_TESTING.md`.

## Licenciamento

Esta governança não substitui nem normaliza licenças. Consulte `LICENSING.md` e os identificadores SPDX/copyrights por arquivo.
