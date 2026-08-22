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
---------------------------------

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
------------

Direct runtime path validated on Mali-G720
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The direct PanVK/libpoly tessellation path has now executed end-to-end on the
Mali-G720/kbase/CSF development target.

The validated direct sequence is:

.. code-block:: text

   software VS
       |
       v
   TCS
       |
       v
   libpoly topology COUNT
       |
       v
   tessellation prefix sum
       |
       v
   topology WITH_COUNTS
       |
       v
   generated indexed indirect draw
       |
       v
   TES / IDVS
       |
       v
   rasterization / framebuffer

Important implementation fixes validated during bring-up:

- software-VS padded compute invocations are guarded with NIR control flow,
  preventing WG64 padding lanes from writing beyond the logical vertex output
  allocation;
- the generic libpoly implementation remains unchanged for that guard;
- TES output bases are recomputed after ``poly_nir_lower_tes()`` because
  point-mode lowering can introduce the default point-size output after the
  original TES IO assignment;
- the fix uses the existing generic
  ``nir_recompute_io_bases(nir, nir_var_shader_out)`` helper;
- the libpoly heap/index contract was verified: the hardware index-buffer base
  is the poly heap base and the generated indirect ``firstIndex`` carries the
  allocated heap offset.

Hardware validation completed
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following focused tests passed on Mali-G720 MC8:

- direct SW-VS -> TCS -> COUNT -> PREFIX -> WITH_COUNTS -> TES/IDVS execution;
- final normal rasterized tessellation draw with successful CSF completion;
- 64x64 framebuffer semantic oracle: all 4096 pixels matched the CPU triangle
  reference;
- asymmetric triangle point-mode oracle using
  ``inner=3, outer={2,3,4}``: all 12 expected tessellation-coordinate points
  matched;
- TES user-defined IO -> fragment-shader validation: all 12 point colors
  matched ``gl_TessCoord``;
- ``fractional_even_spacing`` parity-aligned triangle case: 19/19 expected
  points, no missing/extra/color mismatches;
- ``fractional_odd_spacing`` parity-aligned triangle case: 29/29 expected
  points, no missing/extra/color mismatches;
- quad point-mode case ``inner={4,3}, outer={2,3,4,6}``: 21/21 points;
- isoline point-mode case ``outer={4,6}``: 28/28 points.

These tests establish functional direct-path execution and semantic correctness
for the cases above. They are not Vulkan conformance claims.

Feature exposure and remaining work
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``tessellationShader`` remains ``false`` in the published checkpoint.

The full path was activated temporarily for focused hardware validation and is
kept gated while the remaining integration work is completed. In particular,
the project still needs:

- audit and validation of application indirect tessellation draws;
- multiple sequential tessellation draws and complete PanVK dirty-state
  handling;
- query/XFB and other surrounding graphics-state interactions;
- simultaneous-use / command-buffer lifetime audit;
- winding, patch-discard, limits and invariance coverage;
- fractional-spacing property testing for non-integer levels where exact
  tessellation coordinates are implementation-defined;
- focused regression testing and broader Vulkan CTS coverage;
- cleanup of remaining development diagnostics before feature advertisement.

``vertexPipelineStoresAndAtomics`` is also not force-enabled on this G720
checkpoint merely to run tessellation tests.

The feature will only be advertised after these remaining areas are validated.

Unsupported / deferred features (current state)
-----------------------------------------------
- geometryShader — disabled
- tessellationShader — disabled; direct path hardware-validated, integration/conformance work remains
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
-----------------------------------------------

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

`Mesa <https://mesa3d.org>`_ - The 3D Graphics Library
======================================================


Source
------

This repository lives at https://gitlab.freedesktop.org/mesa/mesa.
Other repositories are likely forks, and code found there is not supported.


Build & install
---------------

You can find more information in our documentation (`docs/install.rst
<https://docs.mesa3d.org/install.html>`_), but the recommended way is to use
Meson (`docs/meson.rst <https://docs.mesa3d.org/meson.html>`_):

.. code-block:: sh

  $ meson setup build
  $ ninja -C build/
  $ sudo ninja -C build/ install

Support
-------

Many Mesa devs hang on IRC; if you're not sure which channel is
appropriate, you should ask your question on `OFTC's #dri-devel
<irc://irc.oftc.net/dri-devel>`_, someone will redirect you if
necessary.
Remember that not everyone is in the same timezone as you, so it might
take a while before someone qualified sees your question.
To figure out who you're talking to, or which nick to ping for your
question, check out `Who's Who on IRC
<https://dri.freedesktop.org/wiki/WhosWho/>`_.

The next best option is to ask your question in an email to the
mailing lists: `mesa-dev\@lists.freedesktop.org
<https://lists.freedesktop.org/mailman/listinfo/mesa-dev>`_


Bug reports
-----------

If you think something isn't working properly, please file a bug report
(`docs/bugs.rst <https://docs.mesa3d.org/bugs.html>`_).


Contributing
------------

Contributions are welcome, and step-by-step instructions can be found in our
documentation (`docs/submittingpatches.rst
<https://docs.mesa3d.org/submittingpatches.html>`_).

Note that Mesa uses gitlab for patches submission, review and discussions.
