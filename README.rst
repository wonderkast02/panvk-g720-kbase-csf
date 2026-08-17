PanVK Mali-G720 / kbase / CSF
=============================

Project scope
-------------
This repository contains work to adapt Mesa/PanVK to a native Kbase/CSF path
for Arm Mali-G720 (MediaTek MT6899). The current focus is a native AArch64
driver stack that runs PanVK directly over the proprietary kbase/CSF kernel
interface, without a Vulkan wrapper in the primary driver path.

Current architecture
--------------------

The current development focuses on a native path without wrapper dependencies:

.. code-block:: text

   Vulkan application
          |
          v
   Vulkan loader
          |
          v
   PanVK
          |
          v
   kbase / CSF
          |
          v
   Mali-G720

Wine, DXVK, Box64 and the historical wrapper are outside the native driver
path and are not required by the current PanVK/kbase architecture.

Historical Proof of Concept (PoC)
--------------------------------

The PoC validated an indirect Vulkan execution path through the bionic-to-glibc
wrapper, allowing early validation of PanVK over kbase/CSF on the Mali-G720.

What it validated:

- Vulkan dispatch through the wrapper path and successful execution through the
  PanVK -> kbase/CSF -> Mali-G720 path.

The full historical snapshot is preserved at:

``porting/g720-wrapper-glibc-snapshot/README.md``

.. code-block:: text

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

Hardware target (confirmed)
---------------------------
- SoC: MediaTek MT6899
- GPU: Mali-G720 MC8
- Android: 16
- kbase: r49p1
- UK: 1.30
- Device node: /dev/mali0
- GPU ID: 0xc8700010
- vendor: 0x13b5
- CSF global version: 0x03060000
- CSF groups: 8
- CSF streams: 64
- instruction features: 0x71

Vulkan validation
-----------------
The following workloads have been validated on the current G720 development
target:

- vulkaninfo
- compute
- graphics preflight
- offscreen triangle + readback
- texture sampling
- D32 depth
- D24S8 stencil
- blending
- MSAA resolve
- stress testing
- ARM64 X11 swapchain
- 300-frame run
- vkcube

Do not interpret these results as Vulkan conformance or full Vulkan support.

G720 CS register fix
--------------------

- Commit: a6799dbeb02 — panvk/kbase: use CSF-reported register count for G720

Key changes:

.. code-block:: c

  .cs_reg_count = (stream_features & 0xff) + 1
  .nr_kernel_registers =
     MAX2(csif_info->unpreserved_cs_reg_count, 4)

Explanation:

Using the CSF-reported register count correctly sizes the available CS register
file and avoids the "overflowed register file" failure observed with the
previous fixed assumption.

Tessellation
-----------

Completed
^^^^^^^^^

- VS -> COMPUTE
- TCS -> COMPUTE
- TES -> VERTEX
- TCS/TES metadata
- TCS/TES binding/state
- TCS/TES descriptors
- poly sysvals ABI
- libpoly ABI audit
- Physical64 ptr_size fix
- libpan precompiled tessellation kernels

Relevant commits:

- 8edb5d94746
- 2aef87163bf

Precompiled kernels added:

- panlib_prefix_sum_tess
- panlib_tess_isoline
- panlib_tess_tri
- panlib_tess_quad

The mesa_clc -> SPIR-V -> panfrost_compile pipeline successfully generates
the v10 kernels for these precompiled tessellation entry points.

Runtime work in progress
^^^^^^^^^^^^^^^^^^^^^^^^

- ``tessellationShader = false`` (will remain false until runtime path is complete)

Runtime work still required:

- poly heap
- per-draw buffers
- poly_vertex_params
- poly_tess_params
- gfx.sysvals.poly
- software VS dispatch
- TCS dispatch
- COUNT
- prefix sum
- WITH_COUNTS
- final TES indexed indirect draw
- runtime validation

Tessellation will only be enabled (``tessellationShader = true``) after the
complete runtime path has been implemented and validated.

Unsupported / deferred features (current state)
----------------------------------------------
- geometryShader — disabled
- tessellationShader — disabled, runtime implementation in progress
- multiViewport — disabled
- textureCompressionBC — unsupported on this hardware path
- shaderClipDistance — deferred
- shaderCullDistance — deferred

Quick start
-----------

Set minimal environment (``DISPLAY`` must match the active X server session):

.. code-block:: sh

  export DISPLAY=:0
  export VK_DRIVER_FILES=/usr/share/vulkan/icd.d/panfrost_kbase_icd.aarch64.json
  export WSI_X11_TERMUX=1

Examples:

.. code-block:: sh

  vulkaninfo --summary
  vkmark --winsys xcb --present-mode immediate

For full installation, WSI and troubleshooting details see:
``docs/panvk-kbase.md`` and ``.github/panvk-kbase-release-notes.md``.

Installation, WSI and kernel warnings (summary)
----------------------------------------------

- Installation: release packages and platform instructions are documented in
  ``docs/panvk-kbase.md`` and in the repository release notes.
- X11 / WSI: raw dma-buf Termux:X11 path and MIT-SHM fallback are documented in
  ``docs/panvk-kbase.md``.
- Kernel warning: running the proprietary kbase driver in containers (chroot,
  proot, DroidSpaces) can expose kernel bugs and may require device-specific
  kernel fixes; see ``.github/panvk-kbase-release-notes.md`` for references.

Development policy
------------------

Vulkan feature bits are only advertised after implementation and runtime
validation.

Upstream Mesa
-------------
The upstream README content follows below and is preserved unmodified.
