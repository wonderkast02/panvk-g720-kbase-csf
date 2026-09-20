# Política de versionamento

A release pública atual é **`0.1.0-beta.1.9.4`**.

Ela é uma **GitHub Pre-release**, não uma release estável.

## Regras

- artefatos internos/debug/test após a beta usam nomes técnicos descritivos;
- número de beta pública só é atribuído quando um build é explicitamente selecionado para distribuição;
- uma atualização de README/docs não altera versão pública;
- fechar uma feature em desenvolvimento não cria automaticamente release;
- bytes já publicados nunca são substituídos silenciosamente sob a mesma versão;
- se os bytes do driver/pacote mudarem, uma futura publicação deve receber nova identidade;
- tags e hashes publicados permanecem imutáveis;
- não criar `beta.2`, `beta.3` ou qualquer sequência pública apenas para numerar trabalho interno.

## Binding da beta atual

- tag: `0.1.0-beta.1.9.4`
- source commit: `3549264275c9663ed73e01d652f4c0d16f21df22`
- package SHA-256: `01c6304206c6e348cb069e3d04fb1c7b693195b543b4134ad7c108a33906d1fa`

O desenvolvimento pós-beta é deliberadamente separado dessa identidade até uma nova decisão de release.
