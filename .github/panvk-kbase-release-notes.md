# PanVK G720 Android candidate notes

Status: **development candidate / community distribution HOLD**.

The current candidate identity is `0.1.0-beta.1.9.4`. It is not a Git tag or GitHub Release and is not described as stable or Vulkan conformant.

## Source lineage

This branch preserves the tracked source lineage used by the validated Android/Bionic staging:

1. exact historical `ci` checkpoint `0521a3257628e811cfead6b5a9753e9f705e2f31`;
2. two Android/Bionic staging deltas (`panvk_device.h`, `u_gralloc/meson.build`);
3. eight validated MC8 source deltas;
4. exact recovered R3 AIMapper v5 FullPlane transformation;
5. Kbase dma-heap **device node** `O_RDONLY | O_CLOEXEC` change while returned dma-bufs remain `O_RDWR | O_CLOEXEC`.

The Beta2 WSI/dma-buf-decoupling and Beta3 explicit-exportable-routing experiments are not part of this candidate branch.

## Validation boundary

Native MC8 qualification of the FullPlane + O_RDONLY composition passed the R3F fake-swapchain, bridge and core matrices. The later standard-wrapper Gate A for `0.1.0-beta.1.9.4` reached its first `vkQueueSubmit` but did not reach acquire/present. The observed `_wassert` alone is not proof of a GPU fatal; the prepared R3H forensic stage had not been executed at this publication freeze.

`tessellationShader=true` is validated on the authoritative Mali-G720 MC8 development line within the recorded directed/CTS scope. This is **not** a Vulkan conformance or universal G720 support claim.

## CI policy

`.github/workflows/build.yml` intentionally performs **source-provenance verification only** for this candidate. It does not upload a driver binary. The previous generic API30/X11 wrapper-labelled artifact workflow did not reproduce the validated physical Android/Bionic API35 build environment and is therefore not used for release evidence.

The independent `deqp-vk-aarch64.yml` tool-builder is retained unchanged.
