# Política de versionamento

A partir da Beta 2, o projeto segue **Semantic Versioning 2.0.0** de forma convencional.

Release pública atual:

- title: **`PanVK G720 0.1.0 Beta 2`**
- tag: **`0.1.0-beta.2`**

## Sequência de pre-release

Durante a linha `0.1.0`:

```text
0.1.0-beta.2
0.1.0-beta.3
0.1.0-beta.4
...
0.1.0-rc.1
0.1.0-rc.2
...
0.1.0
```

Não usar novos contadores aninhados como `beta.1.10.0`.

## Regras

- `beta.N` identifica uma beta pública real, não um build interno;
- artefatos internos/debug/test usam nomes técnicos descritivos;
- uma atualização de README/docs não cria nova versão;
- fechar uma feature em desenvolvimento não cria automaticamente release;
- bytes publicados nunca são substituídos silenciosamente sob a mesma tag;
- tags e hashes publicados permanecem imutáveis;
- uma nova beta exige candidato congelado, proveniência, qualificação e autorização explícita;
- `rc.N` só é usado quando o projeto estiver realmente em fase de release candidate;
- `0.1.0` sem sufixo será uma release estável somente mediante decisão explícita e qualificação adequada.

## GitHub Release class

- `beta.N`: `prerelease=true`
- `rc.N`: `prerelease=true`
- stable: `prerelease=false`

GitHub não permite que uma pre-release seja marcada como `Latest`.

## Histórico legado

`0.1.0-beta.1.9.4` é uma tag histórica válida e permanece intacta. Ela não será renomeada retroativamente.

Sob precedência SemVer, `0.1.0-beta.2` é posterior a `0.1.0-beta.1.9.4` porque o primeiro identificador numérico diferente é `2 > 1`.

## Binding atual

- tag: `0.1.0-beta.2`
- source snapshot: `f1d7bed571766c49e5dd464f92d1fda264612311`
- package SHA-256: `fc1d69647c071ca3fe30ae2fb450e95c91c08e90779eb32c865fa384dff5aaca`
