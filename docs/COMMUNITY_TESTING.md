# Guia de testes da comunidade

## Beta pública atual

Release: **`0.1.0-beta.1.9.4`**

Pacote oficial:

`PanVK-G720-0.1.0-beta.1.9.4.zip`

SHA-256 esperado:

`01c6304206c6e348cb069e3d04fb1c7b693195b543b4134ad7c108a33906d1fa`

Verifique antes do teste:

```sh
sha256sum PanVK-G720-0.1.0-beta.1.9.4.zip
```

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
9. versão do wrapper, quando aplicável;
10. DXVK/VKD3D, quando aplicável;
11. aplicação/jogo;
12. passos exatos para reprodução;
13. logs com timestamps;
14. se houve reboot, abort da aplicação, hang, device lost ou apenas falha do processo de teste.

## Builds de desenvolvimento

Se um build de desenvolvimento for compartilhado para teste dirigido, registre também:

- nome técnico exato do artefato;
- SHA-256 do pacote;
- feature/boundary em teste;
- diferença em relação à beta pública.

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
