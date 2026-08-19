Experimental PanVK Vulkan packages for arm64 Linux environments using the
proprietary Arm Mali **kbase CSF** kernel driver. This build does not use the
upstream panthor/panfrost DRM kernel interface.

## Packages

- Debian 13: `mesa-panvk-kbase-debian13_*_arm64.deb`
- Ubuntu 26.04: `mesa-panvk-kbase-ubuntu2604_*_arm64.deb`
- Arch Linux: `mesa-panvk-kbase-*-aarch64.pkg.tar.zst`
- Generic arm64: `mesa-panvk-kbase-*-aarch64.tar.gz`
- `SHA256SUMS` for all packages

Install a distro package with `apt install ./PACKAGE.deb` or
`pacman -U ./PACKAGE.pkg.tar.zst`. For the generic archive, extract it, enter
the extracted directory, and run `. ./env.sh` from Bash.

## Tested GPUs

- Mali-G710 MC7 in Google Tensor G2 (Pixel 7): `vulkaninfo`, `vkcube`,
  `vkmark`, Termux:X11 presentation, and Minecraft 26.2's native Vulkan
  backend at 1920x1080.
- Mali-G925 (reported as G725 by the tested platform): `vulkaninfo`, `vkmark`,
  and X11 presentation.
- Mali-G720 MC8 in MediaTek MT6899: `vulkaninfo`, compute, graphics,
  offscreen triangle/readback, texture sampling, D32, D24S8, blending,
  MSAA resolve, stress testing, Termux:X11 swapchain, a 300-frame run,
  and `vkcube`.

## Mali-G720 development status

The G720 development branch uses PanVK directly over the proprietary kbase/CSF
interface. The current native path does not require a Vulkan wrapper.

Tessellation compiler plumbing and direct-draw runtime groundwork are under
active development. Build-validated work currently reaches the physical
software-VS compute dispatch, a CSF wait dependency, and the TCS compute
dispatch, with per-command-buffer poly heap and per-draw libpoly parameter
storage already wired.

`tessellationShader` remains disabled. Tessellator execution, TES/final indexed
draw execution, hardware runtime validation, indirect tessellation, and safe
simultaneous execution of the same tessellation command buffer remain pending.

## Basic usage

```sh
export DISPLAY=:0
export VK_DRIVER_FILES=/usr/share/vulkan/icd.d/panfrost_kbase_icd.aarch64.json
export WSI_X11_TERMUX=1

vulkaninfo --summary
vkmark --winsys xcb --present-mode immediate
```

`PANVK_KBASE_DRI3=raw` remains available as the PanVK-specific spelling.
`PANVK_KBASE_DRI3=0` explicitly selects the SHM fallback.

Minecraft 26.2 can use its native Vulkan backend with the same environment.
In Prism Launcher select **Prefer Vulkan (Experimental)**, or launch an
existing instance directly:

```sh
prismlauncher -l 26.2
```

This release advertises the vertex attribute divisor and non-solid-fill
capability gates required during Minecraft's Vulkan device selection.  The
kbase tiler heap is renewed conservatively to prevent the queue-group OOM that
previously surfaced as `VK_ERROR_DEVICE_LOST` after entering a world.

Change `DISPLAY` to match the Termux:X11/Winlator session. The raw WSI path
requires access to `/dev/dma_heap/system`; override it when necessary with
`PANVK_KBASE_DMA_HEAP`. Without either raw-WSI switch, X11 presentation uses
the much slower MIT-SHM fallback.

## Container kernel warning

**Using the proprietary kbase driver from chroot, proot, DroidSpaces, or a
similar container can trigger kernel-driver bugs, including a kernel panic or
device reboot. Some devices require a kernel patch. Review and port any fix for
the exact device and kernel before flashing it.**

- GKI 2.0 reference: https://github.com/funnymdzz/mali_fxxker
- Non-GKI reference: https://github.com/yoshi3jp/android_kernel_samsung_a25ex_mt6835/commit/87003e599eaf2b67bb93523021ca70e7c51f7dca

Full installation, WSI design, performance notes, and troubleshooting are in
`docs/panvk-kbase.md` in the source tree.
