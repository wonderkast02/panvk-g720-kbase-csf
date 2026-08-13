# Mali-G720 Kbase CSF Proof of Concept

Este repositório documenta a prova de conceito de **execução de comandos na GPU Mali-G720** através da interface **Kbase/CSF** no Android, sem depender do driver Vulkan proprietário.

## 🚀 Conquistas

- ✅ Inicialização do contexto Kbase via `/dev/mali0` com `VERSION_CHECK` e `SET_FLAGS`.
- ✅ Alocação de memória GPU com `BASE_MEM_SAME_VA`.
- ✅ Criação de CSG (Command Stream Group) e fila CSF.
- ✅ Bind da fila e mapeamento das páginas de interface (doorbell, input, output).
- ✅ Escrita nos registradores de controle (32 bits).
- ✅ Emissão de comandos CSF reais: `MOVE48`, `MOVE32`, `SYNC_SET32`, `LOAD_MULTIPLE`, `STORE_MULTIPLE`, `WAIT`.
- ✅ **Prova de escrita em memória pela GPU** (`test_gpu_write.c`).
- ✅ **Prova de cópia de memória pela GPU** (`test_gpu_copy_v2.c`).
- ✅ Mapeamento da UAPI Kbase r49p1.

## 📂 Principais arquivos

| Arquivo | Descrição |
|---------|-----------|
| `libkbase_csf.h/.c` | Biblioteca que encapsula inicialização, alocação e submissão. |
| `test_gpu_write.c` | Teste que faz a GPU escrever `0xDEADBEEF` em um buffer. |
| `test_gpu_copy_v2.c` | Teste que faz a GPU copiar 4 bytes usando `LOAD_MULTIPLE`/`STORE_MULTIPLE` com `WAIT`. |
| `test_backend.c` | Teste da camada de abstração `kbase_kmod_test`. |
| `pan_kmod_kbase.c` | Backend experimental para integração com o Mesa/PanVK. |
| `include/` | Headers da UAPI Kbase usados para compilar os testes. |

## 🔧 Compilação dos testes

### Requisitos

- Termux com `clang` instalado.
- Headers do Kbase copiados em `include/`.

### Comandos de exemplo

```bash
clang -I include -I . libkbase_csf.c test_gpu_write.c -o test_gpu_write
./test_gpu_write

clang -I include -I . libkbase_csf.c test_gpu_copy_v2.c -o test_gpu_copy_v2
./test_gpu_copy_v2

## 🙏 Créditos

- **Icecream95** – Autor original do `pan_base` e da engenharia reversa da UAPI kbase.
- **Panfork** (gitlab.com/panfork/mesa) – Mantenedores atuais da camada `pan_base`.
- **Saikatsaha1996** (mesa-Panfrost-G610) – Fork com suporte a G610/G710 e contribuições para CSF.
- **Comunidade Panfrost** – Pelo trabalho contínuo no driver open‑source.

Este projeto utiliza sequências de ioctls e flags inspiradas no Panfork, conforme as notas técnicas.

## 🙏 Créditos

- **Icecream95** – autor original do `pan_base` e engenharia reversa da UAPI kbase
- **Panfork** (gitlab.com/panfork/mesa) – mantenedores da camada `pan_base`
- **Saikatsaha1996** (mesa-Panfrost-G610) – contribuições para G610/G710
- **wonderkast02** – adaptações para Mali-G720
