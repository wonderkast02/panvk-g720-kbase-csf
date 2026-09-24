# Guia de testes da comunidade

## Beta pública atual

Release: **`0.1.0-beta.2`**

Release page:
https://github.com/wonderkast02/panvk-g720-kbase-csf/releases/tag/0.1.0-beta.2

Pacote oficial:

`PanVK-G720-0.1.0-beta.2.zip`

SHA-256 esperado:

`fc1d69647c071ca3fe30ae2fb450e95c91c08e90779eb32c865fa384dff5aaca`

Verifique antes do teste:

```sh
sha256sum PanVK-G720-0.1.0-beta.2.zip
```

O resultado deve ser exatamente o hash acima.

Não repacke ou edite o ZIP quando o objetivo for produzir um resultado atribuível à release oficial.

## Relatório mínimo

Inclua:

1. modelo do dispositivo;
2. SoC;
3. GPU e quantidade de cores quando conhecida;
4. versão do Android;
5. kernel/Kbase quando conhecido;
6. tag exata do PanVK;
7. SHA-256 exato do pacote;
8. runtime: native / Winlator / Vortek / Wine / Box64;
9. versão do wrapper/Winlator quando aplicável;
10. DXVK/VKD3D quando aplicável;
11. aplicação/jogo;
12. resolução e configurações relevantes;
13. passos exatos para reprodução;
14. logs com timestamps;
15. se houve reboot, abort da aplicação, hang, device lost, artefato visual ou apenas falha do processo de teste;
16. se o problema reproduz com instrumentação/logging desativada.

## Problemas de renderização e performance

Para artefatos, glitches ou quedas de FPS:

- descreva a cena exata;
- informe se o problema é determinístico;
- capture frametime quando possível;
- compare apenas ambientes que diferem em uma variável controlada;
- não trate FPS alto como sucesso quando a imagem está incorreta;
- não atribua automaticamente a causa ao PanVK sem evidência.

## Builds de desenvolvimento

Se um build interno for compartilhado para teste dirigido, registre também:

- nome técnico exato do artefato;
- SHA-256 do pacote;
- feature/boundary em teste;
- diferença em relação à Beta 2.

Não descreva um build interno como nova beta sem publicação oficial.

## Interpretação

- assertion de aplicação != GPU fatal automaticamente;
- `VkResult` não-zero != GPU fatal automaticamente;
- tooling failure != driver failure;
- transporte indisponível != driver failure;
- preserve logs originais sem editar.

## Segurança

Testes de driver GPU experimental podem travar aplicações, faultar a GPU ou exigir reboot. Salve trabalhos importantes antes de testar.

Nunca anexe tokens, senhas, cookies, chaves privadas, dumps com dados pessoais desnecessários ou outros segredos.
