# PanVK on the proprietary Mali kbase driver

This branch provides an experimental PanVK Vulkan ICD for Arm Mali CSF GPUs
using the proprietary `kbase` kernel driver. It does **not** use the upstream
`panthor` or legacy `panfrost` DRM kernel interfaces. It is intended for arm64
Linux environments such as chroot, proot, DroidSpaces, and similar containers
running on Android, provided that the required device nodes are accessible.

This is an experimental driver. Support has only been confirmed on the devices
listed below and should not be interpreted as support for every Mali GPU or
kbase kernel release.

## KRAID shader compiler

Release packages include Mesa's upstream KRAID Rust shader compiler for
Valhall and newer Mali GPUs. Upstream keeps compiler selection opt-in while
coverage is expanded. Select stages with `PAN_USE_KRAID`; for example:

```sh
PAN_USE_KRAID=all vulkan-app
```

Stage names such as `vs`, `fs`, `cs`, and `internal` can be combined. Leave
the variable unset to use the established Bifrost compiler path.

## Tested hardware

- Mali-G710 MC7 in Google Tensor G2 (Pixel 7), with Debian 13 in DroidSpaces.
  `vulkaninfo`, `vkcube`, `vkmark`, X11 presentation, and Minecraft 26.2's
  native Vulkan backend at 1920x1080 have been tested.
- Mali-G925 (reported as G725 by the tested platform). `vulkaninfo` and
  `vkmark`, including X11 presentation, have been tested.

## Mali-G720 development checkpoint

The current public community-test prerelease is `0.1.0-beta.2`
(`PanVK G720 0.1.0 Beta 2`).  Its tag points to
`f1d7bed571766c49e5dd464f92d1fda264612311`, which is a documentation-only
child of the qualified technical source commit
`980ac91de74df5e5807e6269fd2531fa3ee6b4e5`.  The distributed driver reuses
the byte-exact G4-B qualified artifact; publishing the documentation child did
not rebuild the driver.

The historical `0.1.0-beta.1.9.4` release remains immutable.  Starting with
Beta 2, public prereleases follow the conventional SemVer sequence
`0.1.0-beta.N`.

The internal GPU-wait promotion landed in commit
`980ac91de74df5e5807e6269fd2531fa3ee6b4e5`.  In the current development
source:

- `geometryShader` and `tessellationShader` are advertised by the PanVK
  physical-device feature table;
- the direct G720 tessellation path has focused hardware and semantic
  validation, while broader CTS/conformance and edge-case coverage continues;
- local/internal PanVK binary-semaphore dependencies can use the native GPU
  WAIT64 path described below.

These are development-branch facts, not Vulkan conformance or universal
compatibility claims.

### Internal GPU WAIT64 synchronization

For eligible local/internal PanVK binary payloads, the kbase submit path
collects up to three internal wait cells and emits CS `SYNC64` waits with the
validated `GREATER(target - 1)` condition.  The waits are emitted before
resource acquisition so a blocked consumer does not reserve execution
resources required by its producer.

The native path is intentionally restricted.  Imported `sync_file` payloads,
timeline wrappers, mixed wait sets, wait-only submits and oversized wait sets
continue to use the existing CPU/KCPU bridge.  External synchronization
semantics are therefore not replaced by this optimization.

For diagnostics, the native internal-wait path can be disabled without changing
the external fallback behavior:

```sh
export PANVK_KBASE_GPU_INTERNAL_WAITS=0
```

## Requirements

- An arm64 Linux userspace and Vulkan loader.
- A proprietary Mali kbase CSF kernel driver and access to `/dev/mali0`.
- Access to `/dev/dma_heap/system` for the fast raw dma-buf X11 WSI path.
- An X server supporting the Termux:X11/Winlator private DRI3 protocol when
  using raw dma-buf presentation.
- Device-node permissions that allow the container user to open the Mali and
  dma-heap devices.

## Installation

Releases contain four package variants:

- Debian 13: `mesa-panvk-kbase-debian13_*_arm64.deb`
- Ubuntu 26.04: `mesa-panvk-kbase-ubuntu2604_*_arm64.deb`
- Arch Linux: `mesa-panvk-kbase-*-aarch64.pkg.tar.zst`
- Generic arm64: `mesa-panvk-kbase-*-aarch64.tar.gz`

Install the matching native package:

```sh
# Debian 13 or Ubuntu 26.04
sudo apt install ./mesa-panvk-kbase-*_arm64.deb

# Arch Linux
sudo pacman -U ./mesa-panvk-kbase-*-aarch64.pkg.tar.zst
```

The generic archive is relocatable. Extract it and source its environment file
from Bash:

```sh
tar -xzf mesa-panvk-kbase-*-aarch64.tar.gz
cd mesa-panvk-kbase-*-aarch64
. ./env.sh
```

## Running Vulkan applications

The distro packages install the ICD manifest system-wide. Selecting it
explicitly is useful when another Vulkan driver is also installed:

```sh
export DISPLAY=:0                 # Change this for the X server in use.
export VK_DRIVER_FILES=/usr/share/vulkan/icd.d/panfrost_kbase_icd.aarch64.json
export WSI_X11_TERMUX=1

vulkaninfo --summary
vkmark --winsys xcb --present-mode immediate
```

`WSI_X11_TERMUX=1` selects PanVK's raw-FD Termux:X11 path when no explicit
PanVK override is present.  `PANVK_KBASE_DRI3=raw` remains available, while
`PANVK_KBASE_DRI3=0` explicitly selects the SHM fallback.

Minecraft 26.2 can use the same environment with its native Vulkan backend.
In Prism Launcher select **Prefer Vulkan (Experimental)**, or launch an
existing instance directly:

```sh
prismlauncher -l 26.2
```

OpenGL applications can run through Zink when a compatible Zink build is
installed:

```sh
MESA_LOADER_DRIVER_OVERRIDE=zink glmark2
```

Some Termux:X11 installations use a filesystem Unix socket and require a
display value such as `DISPLAY=unix/:0`. Use the value appropriate for the X
server session.

## X11 WSI implementation

Normal Linux DRI3 presentation passes a DRM GEM dma-buf file descriptor to the
X server. The proprietary kbase interface has no DRM device and cannot export a
DRM GEM buffer that way. The fast kbase WSI path instead:

1. allocates exportable swapchain memory from an Android dma-heap;
2. imports that memory into kbase for GPU rendering; and
3. sends the same raw, mmap-able dma-buf to a compatible X server through its
   private DRI3 `PixmapFromBuffers` path.

Set `WSI_X11_TERMUX=1` or `PANVK_KBASE_DRI3=raw` (the alias `termux` is also
accepted) to enable this path. The private raw-buffer modifier value is `1274`.
Termux:X11 and Winlator builds that implement this protocol can present the
buffer without a copy.

The private raw protocol does not provide the normal DRI3 explicit-sync
`FenceFromFD` behavior. PanVK therefore waits for GPU completion before
Present, and Present `IdleNotify` events control when an image may be reused.

Without `PANVK_KBASE_DRI3=raw`, presentation falls back to MIT-SHM. This is
more compatible but can be dramatically slower because rendered images must be
copied through host-visible shared memory. In one Pixel 7 test session, raw
presentation reached 889 FPS at 800x600 and 570 FPS at 1920x1080 in `vkmark`;
the 1920x1080 MIT-SHM path reached 36 FPS. These are device- and session-specific
measurements, not performance guarantees.

`bionic-vulkan-wrapper` uses a different private WSI transport. It sends an
Android `AHardwareBuffer` handle over a Unix socket and identifies that path
with modifier value `1255`. This PanVK implementation does not require the
bionic wrapper: its `1274` path sends a raw dma-buf that the Linux process can
allocate and mmap directly.

The dma-heap path can be overridden if a device exposes a different suitable
heap:

```sh
export PANVK_KBASE_DMA_HEAP=/dev/dma_heap/system
```

## Kernel panic warning

> **Warning:** Running the proprietary kbase driver from chroot, proot,
> DroidSpaces, or another container-like environment can expose kernel-driver
> bugs and may cause a kernel panic or device reboot. Some devices require a
> kernel-side fix. Kernel patches are device- and kernel-specific: review and
> port them carefully, keep backups, and do not flash an unverified kernel.

Reference fixes:

- GKI 2.0 devices: [mali_fxxker](https://github.com/funnymdzz/mali_fxxker)
- Non-GKI example: [Samsung A25 kernel commit
  87003e5](https://github.com/yoshi3jp/android_kernel_samsung_a25ex_mt6835/commit/87003e599eaf2b67bb93523021ca70e7c51f7dca)

These references demonstrate possible fixes; neither patch should be assumed to
apply unchanged to every device.

## Troubleshooting

- For a queue-hang investigation, set `PANVK_DEBUG=kbase_diag` to add GPU
  progress breadcrumbs and command-stream dumps. This instrumentation is off
  by default because its extra memory stores and scoreboard waits reduce
  draw/dispatch throughput.
- If no GPU is found, check access permissions for `/dev/mali0` inside the
  container.
- If raw WSI cannot allocate an image, check `/dev/dma_heap/system` access or
  set `PANVK_KBASE_DMA_HEAP` to the correct heap.
- If the window is blank or the raw path is rejected, confirm that the X server
  supports the Termux:X11/Winlator private DRI3 raw-buffer protocol and that
  `PANVK_KBASE_DRI3=raw` is set.
- If rendering works but presentation is slow, confirm that the process did not
  fall back to MIT-SHM.
