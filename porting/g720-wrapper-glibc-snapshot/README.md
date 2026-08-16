# G720 Wrapper glibc port — checkpoint

Checkpoint do trabalho de portabilidade do
`leegao/bionic-vulkan-wrapper` para Linux/glibc ARM64,
usando PanVK/Kbase como Vulkan ICD subjacente.

## Objetivo

Cadeia pretendida:

```text
DXVK / Vulkan loader
        |
        v
libvulkan_wrapper.so
        |
        v
libvulkan_panfrost.so
        |
        v
kbase CSF
        |
        v
Mali-G720
