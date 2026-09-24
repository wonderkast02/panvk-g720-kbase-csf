/*
 * Copyright © 2021 Collabora Ltd.
 * Copyright © 2026 Google LLC
 * Copyright © 2026 Arm Ltd.
 *
 * Derived from tu_device.c which is:
 * Copyright © 2016 Red Hat.
 * Copyright © 2016 Bas Nieuwenhuizen
 * Copyright © 2015 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 */

#include <sched.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/sync_file.h>

#include "util/disk_cache.h"
#include "util/cnd_monotonic.h"
#include "util/os_misc.h"
#include "util/os_time.h"
#include "util/timespec.h"
#include "util/u_atomic.h"
#include "git_sha1.h"

#include "vk_android.h"
#include "vk_device.h"
#include "vk_drm_syncobj.h"
#include "vk_enum_defines.h"
#include "vk_format.h"
#include "vk_log.h"
#include "vk_physical_device.h"
#include "vk_util.h"
#include "vk_sync.h"

#include "panvk_device.h"
#include "panvk_entrypoints.h"
#include "panvk_image.h"
#include "panvk_instance.h"
#include "panvk_physical_device.h"
#include "panvk_wsi.h"

#include "pan_afbc.h"
#include "pan_props.h"

#include "genxml/gen_macros.h"

#define PER_ARCH_FUNCS(_ver)                                                   \
   void panvk_v##_ver##_get_physical_device_extensions(                        \
      const struct panvk_physical_device *device,                              \
      const struct panvk_instance *instance,                                   \
      struct vk_device_extension_table *ext);                                  \
                                                                               \
   void panvk_v##_ver##_get_physical_device_features(                          \
      const struct panvk_instance *instance,                                   \
      const struct panvk_physical_device *device,                              \
      struct vk_features *features);                                           \
                                                                               \
   void panvk_v##_ver##_get_physical_device_properties(                        \
      const struct panvk_instance *instance,                                   \
      const struct panvk_physical_device *device,                              \
      struct vk_properties *properties);                                       \
                                                                               \
   VkResult panvk_v##_ver##_create_device(                                     \
      struct panvk_physical_device *physical_device,                           \
      const VkDeviceCreateInfo *pCreateInfo,                                   \
      const VkAllocationCallbacks *pAllocator, VkDevice *pDevice);             \
                                                                               \
   void panvk_v##_ver##_destroy_device(                                        \
      struct panvk_device *device, const VkAllocationCallbacks *pAllocator)

PER_ARCH_FUNCS(6);
PER_ARCH_FUNCS(7);
PER_ARCH_FUNCS(10);
PER_ARCH_FUNCS(12);
PER_ARCH_FUNCS(13);
PER_ARCH_FUNCS(14);

static VkResult
create_kmod_dev(struct panvk_physical_device *device,
                const struct panvk_instance *instance, drmDevicePtr drm_device)
{
   const char *path = drm_device->nodes[DRM_NODE_RENDER];
   drmVersionPtr version;
   int fd;

   fd = open(path, O_RDWR | O_CLOEXEC);
   if (fd < 0) {
      return panvk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
                          "failed to open device %s", path);
   }

   version = drmGetVersion(fd);
   if (!version) {
      close(fd);
      return panvk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
                          "failed to query kernel driver version for device %s",
                          path);
   }

   if (strcmp(version->name, "panfrost") && strcmp(version->name, "panthor")) {
      drmFreeVersion(version);
      close(fd);
      return VK_ERROR_INCOMPATIBLE_DRIVER;
   }

   drmFreeVersion(version);

   if (PANVK_DEBUG(STARTUP))
      mesa_logi("Found compatible device '%s'.", path);

   uint32_t flags = PAN_KMOD_DEV_FLAG_OWNS_FD;

   if (PANVK_DEBUG(NO_USER_MMAP_SYNC))
      flags |= PAN_KMOD_DEV_FLAG_MMAP_SYNC_THROUGH_KERNEL;

   device->kmod.dev = pan_kmod_dev_create(fd, flags, &instance->kmod.allocator);

   if (!device->kmod.dev) {
      close(fd);
      return panvk_errorf(instance, VK_ERROR_OUT_OF_HOST_MEMORY,
                          "cannot create device");
   }

   return VK_SUCCESS;
}

#if defined(HAVE_PAN_KMOD_KBASE)
static bool
kbase_sysfs_read(const char *dir, const char *name, char *value,
                 size_t value_size)
{
   char path[192];
   snprintf(path, sizeof(path), "%s/%s", dir, name);

   int fd = open(path, O_RDONLY | O_CLOEXEC);
   if (fd < 0)
      return false;

   ssize_t len = read(fd, value, value_size - 1);
   int saved_errno = errno;
   close(fd);
   errno = saved_errno;
   if (len <= 0)
      return false;

   while (len > 0 && (value[len - 1] == '\n' || value[len - 1] == ' '))
      len--;
   value[len] = '\0';
   return true;
}

static bool
kbase_sysfs_write(const char *dir, const char *name, const char *value)
{
   char path[192];
   snprintf(path, sizeof(path), "%s/%s", dir, name);

   int fd = open(path, O_WRONLY | O_CLOEXEC);
   if (fd < 0)
      return false;

   size_t len = strlen(value);
   ssize_t written = write(fd, value, len);
   int saved_errno = errno;
   close(fd);
   errno = saved_errno;
   return written == len;
}

static bool
kbase_sysfs_has_value(const char *values, const char *wanted)
{
   const char *start = values;

   while (*start) {
      while (*start == ' ' || *start == '\n')
         start++;

      const char *end = start;
      while (*end && *end != ' ' && *end != '\n')
         end++;

      if ((size_t)(end - start) == strlen(wanted) &&
          !strncmp(start, wanted, end - start))
         return true;

      start = end;
   }

   return false;
}

static bool
kbase_parse_frequencies(const char *values, uint32_t requested,
                        uint32_t *min_freq, uint32_t *max_freq,
                        bool *requested_available)
{
   char frequencies[512];
   snprintf(frequencies, sizeof(frequencies), "%s", values);

   *min_freq = UINT32_MAX;
   *max_freq = 0;
   *requested_available = false;

   char *saveptr = NULL;
   for (char *token = strtok_r(frequencies, " \n", &saveptr); token;
        token = strtok_r(NULL, " \n", &saveptr)) {
      char *end = NULL;
      errno = 0;
      unsigned long freq = strtoul(token, &end, 10);
      if (errno || !end || *end || freq > UINT32_MAX)
         return false;

      *min_freq = MIN2(*min_freq, (uint32_t)freq);
      *max_freq = MAX2(*max_freq, (uint32_t)freq);
      *requested_available |= requested == freq;
   }

   return *max_freq != 0;
}

static bool
kbase_set_frequency_range(const char *sysfs, uint32_t min_freq,
                          uint32_t max_freq)
{
   char min_value[32], max_value[32];
   snprintf(min_value, sizeof(min_value), "%u\n", min_freq);
   snprintf(max_value, sizeof(max_value), "%u\n", max_freq);

   /* Lower the minimum first when expanding/down-clocking a range.  Raise
    * the maximum first when selecting a higher fixed frequency. */
   char current_min_value[32];
   uint32_t current_min = 0;
   if (kbase_sysfs_read(sysfs, "scaling_min_freq", current_min_value,
                        sizeof(current_min_value)))
      current_min = strtoul(current_min_value, NULL, 10);

   if (max_freq < current_min) {
      return kbase_sysfs_write(sysfs, "scaling_min_freq", min_value) &&
             kbase_sysfs_write(sysfs, "scaling_max_freq", max_value);
   }

   return kbase_sysfs_write(sysfs, "scaling_max_freq", max_value) &&
          kbase_sysfs_write(sysfs, "scaling_min_freq", min_value);
}

static void
configure_kbase_dvfs(const char *device_path)
{
   const char *option = os_get_option("PANVK_KBASE_DVFS");
   if (!option || !option[0] || !strcmp(option, "none"))
      return;

   const char *device_name = strrchr(device_path, '/');
   device_name = device_name ? device_name + 1 : device_path;

   char sysfs[128];
   snprintf(sysfs, sizeof(sysfs), "/sys/class/misc/%s/device", device_name);

   char available_frequencies[512];
   if (!kbase_sysfs_read(sysfs, "available_frequencies",
                         available_frequencies,
                         sizeof(available_frequencies))) {
      mesa_logw("kbase: PANVK_KBASE_DVFS=%s ignored: cannot read %s", option,
                sysfs);
      return;
   }

   bool auto_mode = !strcmp(option, "auto");
   bool default_mode = !strcmp(option, "default");
   bool max_mode = !strcmp(option, "max");
   uint32_t requested = 0;

   if (!auto_mode && !default_mode && !max_mode) {
      char *end = NULL;
      errno = 0;
      unsigned long parsed = strtoul(option, &end, 10);
      if (errno || !end || *end || parsed > UINT32_MAX)
         goto invalid_option;
      requested = parsed;
   }

   uint32_t min_freq, max_freq;
   bool requested_available;
   if (!kbase_parse_frequencies(available_frequencies, requested, &min_freq,
                                &max_freq, &requested_available)) {
      mesa_logw("kbase: cannot parse available GPU frequencies from %s",
                sysfs);
      return;
   }

   if (max_mode) {
      requested = max_freq;
      requested_available = true;
   }

   if (auto_mode || default_mode) {
      if (!kbase_set_frequency_range(sysfs, min_freq, max_freq))
         goto write_failed;

      const char *governor =
         default_mode ? "quickstep" : "quickstep_use_mcu";
      char available_governors[256];
      if (!kbase_sysfs_read(sysfs, "available_governors",
                            available_governors,
                            sizeof(available_governors)))
         goto write_failed;

      if (auto_mode &&
          !kbase_sysfs_has_value(available_governors, governor))
         governor = "capacity_use_mcu";

      if (!kbase_sysfs_has_value(available_governors, governor)) {
         mesa_logw("kbase: requested DVFS governor is unavailable (%s)",
                   available_governors);
         return;
      }

      char governor_value[64];
      snprintf(governor_value, sizeof(governor_value), "%s\n", governor);
      if (!kbase_sysfs_write(sysfs, "governor", governor_value))
         goto write_failed;

      mesa_logi("kbase: GPU DVFS governor %s, range %u-%u kHz", governor,
                min_freq, max_freq);
      return;
   }

   if (!requested_available) {
      mesa_logw("kbase: requested GPU frequency %u kHz is unavailable (%s)",
                requested, available_frequencies);
      return;
   }

   if (!kbase_set_frequency_range(sysfs, requested, requested))
      goto write_failed;

   mesa_logi("kbase: GPU frequency fixed at %u kHz", requested);
   return;

invalid_option:
   mesa_logw("kbase: invalid PANVK_KBASE_DVFS value '%s' "
             "(expected auto, default, max, none, or a frequency in kHz)",
             option);
   return;

write_failed:
   mesa_logw("kbase: PANVK_KBASE_DVFS=%s could not configure %s: %s", option,
             sysfs, strerror(errno));
}

static VkResult
create_kmod_dev_kbase(struct panvk_physical_device *device,
                      const struct panvk_instance *instance,
                      const char *path)
{
   int fd;

   fd = open(path, O_RDWR | O_CLOEXEC);
   if (fd < 0) {
      return panvk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
                          "failed to open kbase device %s", path);
   }

   if (PANVK_DEBUG(STARTUP))
      mesa_logi("Found kbase device '%s'.", path);

   uint32_t flags = PAN_KMOD_DEV_FLAG_OWNS_FD;

   if (PANVK_DEBUG(NO_USER_MMAP_SYNC))
      flags |= PAN_KMOD_DEV_FLAG_MMAP_SYNC_THROUGH_KERNEL;

   device->kmod.dev = pan_kmod_dev_create_with_driver(fd, flags, "kbase", NULL,
                                                      &instance->kmod.allocator);

   if (!device->kmod.dev) {
      close(fd);
      return panvk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
                          "failed to create kbase device at %s", path);
   }

   snprintf(device->kbase_node_path, sizeof(device->kbase_node_path), "%s",
            path);
   configure_kbase_dvfs(path);

   return VK_SUCCESS;
}
#endif /* HAVE_PAN_KMOD_KBASE */

static VkResult
get_drm_device_ids(struct panvk_physical_device *device,
                   const struct panvk_instance *instance,
                   drmDevicePtr drm_device)
{
   struct stat st;

   if (stat(drm_device->nodes[DRM_NODE_RENDER], &st)) {
      return vk_errorf(instance, VK_ERROR_INITIALIZATION_FAILED,
                       "failed to query render node stat");
   }

   device->drm.render_rdev = st.st_rdev;

   if (drm_device->available_nodes & (1 << DRM_NODE_PRIMARY)) {
      if (stat(drm_device->nodes[DRM_NODE_PRIMARY], &st)) {
         return vk_errorf(instance, VK_ERROR_INITIALIZATION_FAILED,
                          "failed to query primary node stat");
      }

      device->drm.primary_rdev = st.st_rdev;
   }

   return VK_SUCCESS;
}

static void
init_shader_caches(struct panvk_physical_device *device,
                   const struct panvk_instance *instance)
{
   blake3_hasher blake3_ctx;
   _mesa_blake3_init(&blake3_ctx);

   _mesa_blake3_update(&blake3_ctx, instance->driver_build_sha,
                     sizeof(instance->driver_build_sha));

   _mesa_blake3_update(&blake3_ctx, &device->kmod.dev->props.gpu_id,
                     sizeof(device->kmod.dev->props.gpu_id));

   unsigned char blake3[BLAKE3_KEY_LEN];
   _mesa_blake3_final(&blake3_ctx, blake3);

   STATIC_ASSERT(VK_UUID_SIZE <= BLAKE3_KEY_LEN);
   memcpy(device->cache_uuid, blake3, VK_UUID_SIZE);

#ifdef ENABLE_SHADER_CACHE
   char renderer[25];
   ASSERTED int len =
      snprintf(renderer, sizeof(renderer), "panvk_0x%016" PRIx64,
               device->kmod.dev->props.gpu_id);
   assert(len == sizeof(renderer) - 1);

   char timestamp[BLAKE3_HEX_LEN];
   _mesa_blake3_format(timestamp, instance->driver_build_sha);

   const uint64_t driver_flags = 0;
   device->vk.disk_cache = disk_cache_create(renderer, timestamp, driver_flags);
#endif
}

static void
free_disk_cache(struct panvk_physical_device *device)
{
#ifdef ENABLE_SHADER_CACHE
   if (device->vk.disk_cache) {
      disk_cache_destroy(device->vk.disk_cache);
      device->vk.disk_cache = NULL;
   }
#else
   assert(device->vk.disk_cache == NULL);
#endif
}

static VkResult
get_core_mask(struct panvk_physical_device *device,
              const struct panvk_instance *instance, const char *option_name,
              uint64_t opt_mask, uint64_t *mask)
{
   uint64_t present = device->kmod.dev->props.shader_present;
   *mask = opt_mask & present;

   if (!*mask)
      return panvk_errorf(instance, VK_ERROR_INITIALIZATION_FAILED,
                          "None of the cores specified in %s are present. "
                          "Available shader cores are 0x%" PRIx64 ".\n",
                          option_name, present);

   return VK_SUCCESS;
}

static VkResult
get_core_masks(struct panvk_physical_device *device,
               const struct panvk_instance *instance)
{
   VkResult result;

   result = get_core_mask(device, instance, "pan_compute_core_mask",
                          instance->drirc.misc.compute_core_mask,
                          &device->compute_core_mask);
   if (result != VK_SUCCESS)
      return result;
   result = get_core_mask(device, instance, "pan_fragment_core_mask",
                          instance->drirc.misc.fragment_core_mask,
                          &device->fragment_core_mask);

   return result;
}

static VkResult
get_device_heaps(struct panvk_physical_device *device,
                 struct panvk_instance *instance)
{
   int host_coherent_not_cached_idx = -1;
   int host_cached_not_coherent_idx = -1;

   const uint64_t heap_size =
      os_get_gpu_heap_size(instance->drirc.misc.heap_memory_percent,
                           &instance->drirc.misc.heap_memory_percent);

   device->memory.heap_count = 1;
   device->memory.heaps[0] = (VkMemoryHeap){
      .size = heap_size,
      .flags = VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
   };

   device->memory.type_count = 0;

   /* We don't have VRAM, but we expose a device-local only type so we can
    * prevent imported dma-bufs that come from other drivers/subsystems from
    * being CPU-mapped.
    */
   device->memory.types[device->memory.type_count++] = (VkMemoryType) {
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .heapIndex = 0,
   };

   if (device->kmod.dev->props.is_io_coherent) {
      assert(device->memory.type_count < ARRAY_SIZE(device->memory.types));
      /* If the device is coherent, we just have one memory type that's both
       * host-cached and host-coherent. */
      device->memory.types[device->memory.type_count++] = (VkMemoryType) {
         .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_CACHED_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
         .heapIndex = 0,
      };
   }

   if (!PANVK_DEBUG(NO_WB_MMAP) &&
       (device->kmod.dev->props.supported_bo_flags & PAN_KMOD_BO_FLAG_WB_MMAP)) {
      assert(device->memory.type_count < ARRAY_SIZE(device->memory.types));
      host_cached_not_coherent_idx = device->memory.type_count;
      device->memory.types[device->memory.type_count++] = (VkMemoryType) {
         .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
         .heapIndex = 0,
      };
   }

   assert(device->memory.type_count < ARRAY_SIZE(device->memory.types));
   host_coherent_not_cached_idx = device->memory.type_count;
   device->memory.types[device->memory.type_count++] = (VkMemoryType) {
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      .heapIndex = 0,
   };

   /* Ideally, we'd place HOST_CACHED first for perf reasons, but there's
    * so many broken CTS tests (missing or invalid flush/invalidate
    * calls), and so many added at each version that it gets impossible to
    * catch up. So, keep things ordered in a way that the first HOST_VISIBLE
    * type is also the one requiring no CPU cache maintenance if we're asked
    * to.
    */
   if (PANVK_DEBUG(COHERENT_BEFORE_CACHED) &&
       host_cached_not_coherent_idx != -1 &&
       host_coherent_not_cached_idx != -1 &&
       host_coherent_not_cached_idx > host_cached_not_coherent_idx) {
      VkMemoryType host_cached_not_coherent_type =
         device->memory.types[host_cached_not_coherent_idx];

      device->memory.types[host_cached_not_coherent_idx] =
         device->memory.types[host_coherent_not_cached_idx];
      device->memory.types[host_coherent_not_cached_idx] =
         host_cached_not_coherent_type;
   }

   const uint64_t request_va =
      PANVK_DEBUG(NO_EXTENDED_VA_RANGE) ? 1ull << 32 : 1ull << 48;
   device->memory.max_supported_va =
      pan_clamp_to_usable_va_range(device->kmod.dev, request_va);

   return VK_SUCCESS;
}

static VkResult
get_device_sync_types(struct panvk_physical_device *device,
                      const struct panvk_instance *instance)
{
   const unsigned arch = pan_arch(device->kmod.dev->props.gpu_id);
   uint32_t sync_type_count = 0;

   device->drm_syncobj_type = vk_drm_syncobj_get_type(device->kmod.dev->fd);
   if (!device->drm_syncobj_type.features) {
      return vk_errorf(instance, VK_ERROR_INITIALIZATION_FAILED,
                       "failed to query syncobj features");
   }

   device->sync_types[sync_type_count++] = &device->drm_syncobj_type;

   if (arch >= 10) {
      assert(device->drm_syncobj_type.features & VK_SYNC_FEATURE_TIMELINE);
   } else {
      /* We don't support timelines in the uAPI yet and we don't want it getting
       * suddenly turned on by vk_drm_syncobj_get_type() without us adding panvk
       * code for it first.
       */
      device->drm_syncobj_type.features &= ~VK_SYNC_FEATURE_TIMELINE;

      device->sync_timeline_type =
         vk_sync_timeline_get_type(&device->drm_syncobj_type);
      device->sync_types[sync_type_count++] = &device->sync_timeline_type.sync;
   }

   assert(sync_type_count < ARRAY_SIZE(device->sync_types));
   device->sync_types[sync_type_count] = NULL;

   return VK_SUCCESS;
}

#if defined(HAVE_PAN_KMOD_KBASE)

enum kbase_cpu_sync_state {
   KBASE_CPU_SYNC_RESET,
   KBASE_CPU_SYNC_PENDING,
   KBASE_CPU_SYNC_WAITING,
   KBASE_CPU_SYNC_SIGNALED,
   KBASE_CPU_SYNC_FAILED,
};

struct kbase_cpu_sync {
   struct vk_sync sync;
   mtx_t mutex;
   struct u_cnd_monotonic cond;
   enum kbase_cpu_sync_state state;
   VkResult result;
   void *pending_data;
   panvk_kbase_sync_wait_func pending_wait;
   panvk_kbase_sync_export_func pending_export;
   uint64_t targets[PANVK_KBASE_SYNC_TARGET_COUNT];
};

static VkResult
kbase_cpu_sync_init(struct vk_device *device, struct vk_sync *sync,
                    uint64_t initial_value)
{
   struct kbase_cpu_sync *ks = container_of(sync, struct kbase_cpu_sync, sync);
   int ret = mtx_init(&ks->mutex, mtx_plain);
   if (ret != thrd_success)
      return vk_errorf(device, VK_ERROR_UNKNOWN, "mtx_init failed");

   ret = u_cnd_monotonic_init(&ks->cond);
   if (ret != thrd_success) {
      mtx_destroy(&ks->mutex);
      return vk_errorf(device, VK_ERROR_UNKNOWN, "cnd_init failed");
   }

   ks->state = initial_value ? KBASE_CPU_SYNC_SIGNALED : KBASE_CPU_SYNC_RESET;
   ks->result = VK_SUCCESS;
   ks->pending_data = NULL;
   ks->pending_wait = NULL;
   ks->pending_export = NULL;
   memset(ks->targets, 0, sizeof(ks->targets));
   return VK_SUCCESS;
}

static void
kbase_sync_file_waiter_free(struct kbase_cpu_sync *ks);

static void
kbase_cpu_sync_finish(UNUSED struct vk_device *device, struct vk_sync *sync)
{
   struct kbase_cpu_sync *ks = container_of(sync, struct kbase_cpu_sync, sync);
   kbase_sync_file_waiter_free(ks);
   u_cnd_monotonic_destroy(&ks->cond);
   mtx_destroy(&ks->mutex);
}

static VkResult
kbase_cpu_sync_signal(UNUSED struct vk_device *device, struct vk_sync *sync,
                      UNUSED uint64_t value)
{
   struct kbase_cpu_sync *ks = container_of(sync, struct kbase_cpu_sync, sync);
   mtx_lock(&ks->mutex);
   ks->state = KBASE_CPU_SYNC_SIGNALED;
   ks->result = VK_SUCCESS;
   ks->pending_data = NULL;
   ks->pending_wait = NULL;
   ks->pending_export = NULL;
   u_cnd_monotonic_broadcast(&ks->cond);
   mtx_unlock(&ks->mutex);
   return VK_SUCCESS;
}

static VkResult
kbase_cpu_sync_reset(UNUSED struct vk_device *device, struct vk_sync *sync)
{
   struct kbase_cpu_sync *ks = container_of(sync, struct kbase_cpu_sync, sync);
   mtx_lock(&ks->mutex);
   kbase_sync_file_waiter_free(ks);
   assert(ks->state != KBASE_CPU_SYNC_WAITING);
   ks->state = KBASE_CPU_SYNC_RESET;
   ks->result = VK_SUCCESS;
   ks->pending_data = NULL;
   ks->pending_wait = NULL;
   ks->pending_export = NULL;
   memset(ks->targets, 0, sizeof(ks->targets));
   mtx_unlock(&ks->mutex);
   return VK_SUCCESS;
}

static VkResult
kbase_cpu_sync_wait_many(struct vk_device *device, uint32_t wait_count,
                         const struct vk_sync_wait *waits,
                         enum vk_sync_wait_flags wait_flags,
                         uint64_t abs_timeout_ns);

void
panvk_kbase_sync_set_pending(
   struct vk_sync *sync, void *data, panvk_kbase_sync_wait_func wait,
   panvk_kbase_sync_export_func export_sync_file,
   const uint64_t targets[PANVK_KBASE_SYNC_TARGET_COUNT])
{
   struct kbase_cpu_sync *ks = container_of(sync, struct kbase_cpu_sync, sync);

   assert(sync->type->wait_many == kbase_cpu_sync_wait_many);
   mtx_lock(&ks->mutex);
   /* Binary payloads follow DRM-syncobj semantics: a new signal replaces
    * whatever fence the sync currently holds, so re-arming from SIGNALED
    * (e.g. a reused WSI semaphore that was CPU-waited) is legal.  Only an
    * in-progress wait on the old payload would be a genuine bug. */
   assert(ks->state != KBASE_CPU_SYNC_WAITING);
   kbase_sync_file_waiter_free(ks);
   ks->pending_data = data;
   ks->pending_wait = wait;
   ks->pending_export = export_sync_file;
   memcpy(ks->targets, targets, sizeof(ks->targets));
   ks->result = VK_SUCCESS;
   ks->state = KBASE_CPU_SYNC_PENDING;
   u_cnd_monotonic_broadcast(&ks->cond);
   mtx_unlock(&ks->mutex);
}

bool
panvk_kbase_sync_get_pending_payload(
   struct vk_sync *sync,
   struct panvk_kbase_sync_pending_payload *payload)
{
   if (!sync || !payload ||
       sync->type->wait_many != kbase_cpu_sync_wait_many)
      return false;

   struct kbase_cpu_sync *ks = container_of(sync, struct kbase_cpu_sync, sync);

   mtx_lock(&ks->mutex);
   bool pending = ks->state == KBASE_CPU_SYNC_PENDING &&
                  ks->pending_data != NULL &&
                  ks->pending_wait != NULL;
   if (pending) {
      payload->data = ks->pending_data;
      payload->wait = ks->pending_wait;
      payload->export_sync_file = ks->pending_export;
      memcpy(payload->targets, ks->targets, sizeof(payload->targets));
   }
   mtx_unlock(&ks->mutex);

   return pending;
}

static VkResult
kbase_cpu_sync_wait_one(struct vk_device *device, struct kbase_cpu_sync *ks,
                        enum vk_sync_wait_flags wait_flags,
                        uint64_t abs_timeout_ns)
{
   struct timespec abs_timeout_ts;
   timespec_from_nsec(&abs_timeout_ts, abs_timeout_ns);

   mtx_lock(&ks->mutex);
   while (true) {
      switch (ks->state) {
      case KBASE_CPU_SYNC_SIGNALED:
         mtx_unlock(&ks->mutex);
         return VK_SUCCESS;
      case KBASE_CPU_SYNC_FAILED: {
         VkResult result = ks->result;
         mtx_unlock(&ks->mutex);
         return result;
      }
      case KBASE_CPU_SYNC_PENDING: {
         if (wait_flags & VK_SYNC_WAIT_PENDING) {
            mtx_unlock(&ks->mutex);
            return VK_SUCCESS;
         }

         panvk_kbase_sync_wait_func wait = ks->pending_wait;
         void *data = ks->pending_data;
         uint64_t targets[PANVK_KBASE_SYNC_TARGET_COUNT];
         memcpy(targets, ks->targets, sizeof(targets));
         ks->state = KBASE_CPU_SYNC_WAITING;
         mtx_unlock(&ks->mutex);

         VkResult result = wait(data, targets, abs_timeout_ns);

         mtx_lock(&ks->mutex);
         if (result == VK_SUCCESS) {
            kbase_sync_file_waiter_free(ks);
            ks->state = KBASE_CPU_SYNC_SIGNALED;
            ks->pending_data = NULL;
            ks->pending_wait = NULL;
            ks->pending_export = NULL;
         } else if (result == VK_TIMEOUT) {
            ks->state = KBASE_CPU_SYNC_PENDING;
         } else {
            ks->state = KBASE_CPU_SYNC_FAILED;
            ks->result = result;
         }
         u_cnd_monotonic_broadcast(&ks->cond);
         mtx_unlock(&ks->mutex);
         return result;
      }
      case KBASE_CPU_SYNC_RESET:
      case KBASE_CPU_SYNC_WAITING: {
         if (abs_timeout_ns == 0) {
            mtx_unlock(&ks->mutex);
            return VK_TIMEOUT;
         }

         int ret = u_cnd_monotonic_timedwait(&ks->cond, &ks->mutex,
                                              &abs_timeout_ts);
         if (ret == thrd_timedout) {
            mtx_unlock(&ks->mutex);
            return VK_TIMEOUT;
         }
         if (ret != thrd_success) {
            mtx_unlock(&ks->mutex);
            return vk_errorf(device, VK_ERROR_UNKNOWN,
                             "kbase sync condition wait failed");
         }
         break;
      }
      }
   }
}

static VkResult
kbase_cpu_sync_wait_many(struct vk_device *device,
                         uint32_t wait_count, const struct vk_sync_wait *waits,
                         enum vk_sync_wait_flags wait_flags,
                         uint64_t abs_timeout_ns)
{
   bool wait_any = !!(wait_flags & VK_SYNC_WAIT_ANY);

   if (!wait_any) {
      for (uint32_t i = 0; i < wait_count; i++) {
         struct kbase_cpu_sync *ks =
            container_of(waits[i].sync, struct kbase_cpu_sync, sync);
         VkResult result = kbase_cpu_sync_wait_one(
            device, ks, wait_flags, abs_timeout_ns);
         if (result != VK_SUCCESS)
            return result;
      }
      return VK_SUCCESS;
   }

   while (true) {
      for (uint32_t i = 0; i < wait_count; i++) {
         struct kbase_cpu_sync *ks =
            container_of(waits[i].sync, struct kbase_cpu_sync, sync);
         VkResult result = kbase_cpu_sync_wait_one(device, ks, wait_flags, 0);
         if (result == VK_SUCCESS)
            return VK_SUCCESS;
         if (result != VK_TIMEOUT)
            return result;
      }
      if (abs_timeout_ns == 0 || os_time_get_nano() >= abs_timeout_ns)
         return VK_TIMEOUT;
      sched_yield();
   }
}

static VkResult
kbase_cpu_sync_move(UNUSED struct vk_device *device, struct vk_sync *dst,
                    struct vk_sync *src)
{
   if (dst == src)
      return VK_SUCCESS;

   struct kbase_cpu_sync *ks_dst =
      container_of(dst, struct kbase_cpu_sync, sync);
   struct kbase_cpu_sync *ks_src =
      container_of(src, struct kbase_cpu_sync, sync);

   struct kbase_cpu_sync *first =
      (uintptr_t)ks_dst < (uintptr_t)ks_src ? ks_dst : ks_src;
   struct kbase_cpu_sync *second = first == ks_dst ? ks_src : ks_dst;
   mtx_lock(&first->mutex);
   mtx_lock(&second->mutex);
   assert(ks_src->state != KBASE_CPU_SYNC_WAITING);
   assert(ks_dst->state != KBASE_CPU_SYNC_WAITING);
   kbase_sync_file_waiter_free(ks_dst);

   ks_dst->state = ks_src->state;
   ks_dst->result = ks_src->result;
   ks_dst->pending_data = ks_src->pending_data;
   ks_dst->pending_wait = ks_src->pending_wait;
   ks_dst->pending_export = ks_src->pending_export;
   memcpy(ks_dst->targets, ks_src->targets, sizeof(ks_dst->targets));

   ks_src->state = KBASE_CPU_SYNC_RESET;
   ks_src->result = VK_SUCCESS;
   ks_src->pending_data = NULL;
   ks_src->pending_wait = NULL;
   ks_src->pending_export = NULL;
   memset(ks_src->targets, 0, sizeof(ks_src->targets));
   u_cnd_monotonic_broadcast(&ks_dst->cond);
   u_cnd_monotonic_broadcast(&ks_src->cond);
   mtx_unlock(&second->mutex);
   mtx_unlock(&first->mutex);
   return VK_SUCCESS;
}

struct kbase_sync_file_waiter {
   int fd;
};

static VkResult
kbase_sync_file_wait_func(void *data, UNUSED const uint64_t *targets,
                          uint64_t abs_timeout_ns)
{
   struct kbase_sync_file_waiter *waiter = data;
   if (waiter->fd < 0)
      return VK_ERROR_INVALID_EXTERNAL_HANDLE;

   int timeout_ms;
   if (abs_timeout_ns == 0xFFFFFFFFFFFFFFFFULL) {
      timeout_ms = -1;
   } else {
      uint64_t now = os_time_get_nano();
      if (abs_timeout_ns <= now)
         timeout_ms = 0;
      else
         timeout_ms = (int)((abs_timeout_ns - now) / 1000000ull);
   }

   struct pollfd pfd = {
      .fd = waiter->fd,
      .events = POLLIN | POLLERR | POLLHUP,
   };

   while (true) {
      int ret = poll(&pfd, 1, timeout_ms);
      if (ret > 0) {
         if (pfd.revents & (POLLERR | POLLHUP))
            return VK_ERROR_DEVICE_LOST;
         return VK_SUCCESS;
      } else if (ret == 0) {
         return VK_TIMEOUT;
      } else if (errno != EINTR && errno != EAGAIN) {
         return VK_ERROR_UNKNOWN;
      }
   }
}

static void
kbase_sync_file_waiter_free(struct kbase_cpu_sync *ks)
{
   if (ks->pending_wait == kbase_sync_file_wait_func && ks->pending_data) {
      struct kbase_sync_file_waiter *waiter = ks->pending_data;
      if (waiter->fd >= 0)
         close(waiter->fd);
      free(waiter);
      ks->pending_data = NULL;
      ks->pending_wait = NULL;
   ks->pending_export = NULL;
   }
}

static VkResult
kbase_sync_file_export_func(struct vk_device *device, void *data,
   const uint64_t targets[PANVK_KBASE_SYNC_TARGET_COUNT], int *sync_file);

static VkResult
kbase_cpu_sync_import_sync_file(struct vk_device *device,
                                struct vk_sync *sync, int sync_file)
{
   struct kbase_cpu_sync *ks = container_of(sync, struct kbase_cpu_sync, sync);
   if (sync_file == -1) return kbase_cpu_sync_signal(device, sync, 0);
   if (sync_file < -1) return vk_errorf(device, VK_ERROR_INVALID_EXTERNAL_HANDLE, "Invalid sync_file fd: %d", sync_file);
   int fd = fcntl(sync_file, F_DUPFD_CLOEXEC, 0);
   if (fd < 0) return vk_errorf(device, VK_ERROR_OUT_OF_HOST_MEMORY, "sync_file duplicate failed: %m");
   struct kbase_sync_file_waiter *waiter = malloc(sizeof(*waiter));
   if (!waiter) { close(fd); return vk_errorf(device, VK_ERROR_OUT_OF_HOST_MEMORY, "sync_file waiter allocation failed"); }
   waiter->fd = fd; uint64_t targets[PANVK_KBASE_SYNC_TARGET_COUNT] = {0};
   panvk_kbase_sync_set_pending(sync, waiter, kbase_sync_file_wait_func, kbase_sync_file_export_func, targets);
   return VK_SUCCESS;
}

static VkResult
kbase_sync_file_export_func(struct vk_device *device, void *data,
   UNUSED const uint64_t targets[PANVK_KBASE_SYNC_TARGET_COUNT], int *sync_file)
{
   struct kbase_sync_file_waiter *waiter = data;
   if (!waiter || waiter->fd < 0) return vk_errorf(device, VK_ERROR_INVALID_EXTERNAL_HANDLE, "Invalid imported sync_file payload");
   int fd = fcntl(waiter->fd, F_DUPFD_CLOEXEC, 0);
   if (fd < 0) return vk_errorf(device, VK_ERROR_OUT_OF_HOST_MEMORY, "sync_file re-export duplicate failed: %m");
   *sync_file = fd; return VK_SUCCESS;
}

static VkResult
kbase_cpu_sync_export_sync_file(struct vk_device *device, struct vk_sync *sync, int *sync_file)
{
   struct kbase_cpu_sync *ks = container_of(sync, struct kbase_cpu_sync, sync);
   mtx_lock(&ks->mutex);
   enum kbase_cpu_sync_state state = ks->state;
   if (state == KBASE_CPU_SYNC_SIGNALED) {
      mtx_unlock(&ks->mutex);
#ifdef HAVE_PAN_KMOD_KBASE
      struct panvk_device *panvk_dev = to_panvk_device(device);
      struct panvk_physical_device *phys_dev =
         to_panvk_physical_device(panvk_dev->vk.physical);

      if (phys_dev->kbase_node_path[0] && panvk_dev->kmod.dev) {
         if (kbase_kmod_csf_export_signaled_sync_file(panvk_dev->kmod.dev,
                                                      sync_file) == 0)
            return VK_SUCCESS;

         int err = errno;
         VkResult result =
            err == ENOMEM ? VK_ERROR_OUT_OF_HOST_MEMORY :
            (err == EMFILE || err == ENFILE) ? VK_ERROR_TOO_MANY_OBJECTS :
                                               VK_ERROR_UNKNOWN;
         return vk_errorf(device, result,
                          "Kbase signaled sync_file compatibility export failed: %s",
                          strerror(err));
      }
#endif
      /* Panthor and non-Kbase paths keep the standard optional sentinel. */
      *sync_file = -1;
      return VK_SUCCESS;
   }
   if (state == KBASE_CPU_SYNC_FAILED) {
      VkResult result = ks->result; mtx_unlock(&ks->mutex); return result;
   }
   if ((state == KBASE_CPU_SYNC_PENDING || state == KBASE_CPU_SYNC_WAITING) && ks->pending_export) {
      uint64_t targets[PANVK_KBASE_SYNC_TARGET_COUNT]; memcpy(targets, ks->targets, sizeof(targets));
      VkResult result = ks->pending_export(device, ks->pending_data, targets, sync_file);
      mtx_unlock(&ks->mutex); return result;
   }
   mtx_unlock(&ks->mutex);
   return vk_errorf(device,VK_ERROR_UNKNOWN,"Kbase fence payload cannot be exported in state %d",state);
}

static const struct vk_sync_type kbase_cpu_sync_type = {
   .size      = sizeof(struct kbase_cpu_sync),
   .features  = VK_SYNC_FEATURE_BINARY |
                VK_SYNC_FEATURE_GPU_WAIT |
                VK_SYNC_FEATURE_GPU_MULTI_WAIT |
                VK_SYNC_FEATURE_CPU_WAIT |
                VK_SYNC_FEATURE_CPU_RESET |
                VK_SYNC_FEATURE_CPU_SIGNAL |
                VK_SYNC_FEATURE_WAIT_ANY |
                VK_SYNC_FEATURE_WAIT_PENDING,
   .init      = kbase_cpu_sync_init,
   .finish    = kbase_cpu_sync_finish,
   .signal    = kbase_cpu_sync_signal,
   .reset     = kbase_cpu_sync_reset,
   .wait_many = kbase_cpu_sync_wait_many,
   .move      = kbase_cpu_sync_move,
   .import_sync_file = kbase_cpu_sync_import_sync_file,
   .export_sync_file = kbase_cpu_sync_export_sync_file,
};

/* Set up sync types for a kbase (non-DRM) physical device.
 *
 * DRM syncobj is not available on a kbase fd, so we substitute a CPU-based
 * binary sync type and wrap it in a software timeline for architectures that
 * require VK_SYNC_FEATURE_TIMELINE (arch >= 10). */
static VkResult
get_device_sync_types_kbase(struct panvk_physical_device *device,
                             UNUSED const struct panvk_instance *instance)
{
   uint32_t sync_type_count = 0;

   device->drm_syncobj_type = kbase_cpu_sync_type;

   /* Binary CPU sync first (fences need a binary type with CPU_RESET),
    * software timeline emulation second (timeline semaphores).  Submitted
    * syncs carry kbase GPU seqno targets and resolve them on CPU wait. */
   device->sync_types[sync_type_count++] = &device->drm_syncobj_type;

   device->sync_timeline_type =
      vk_sync_timeline_get_type(&device->drm_syncobj_type);
   device->sync_types[sync_type_count++] = &device->sync_timeline_type.sync;

   assert(sync_type_count < ARRAY_SIZE(device->sync_types));
   device->sync_types[sync_type_count] = NULL;

   return VK_SUCCESS;
}

#endif /* HAVE_PAN_KMOD_KBASE */

float
panvk_get_gpu_system_timestamp_period(const struct panvk_physical_device *device)
{
   if (!device->kmod.dev->props.gpu_can_query_timestamp ||
       !device->kmod.dev->props.timestamp_frequency)
      return 0;

   return device->kmod.dev->props.timestamp_cycles_to_ns_factor;
}

void
panvk_physical_device_finish(struct panvk_physical_device *device)
{
   panvk_wsi_finish(device);

   free_disk_cache(device);

   pan_kmod_dev_destroy(device->kmod.dev);

   vk_physical_device_finish(&device->vk);
}

/* Resolve the pan_model for a probed device.  GPUs missing from the model
 * table get conservative per-arch defaults instead of being rejected;
 * *unknown_gpu is set so callers can reflect that in the device name. */
static VkResult
get_gpu_model(struct panvk_physical_device *device,
              const struct panvk_instance *instance, bool *unknown_gpu)
{
   const struct pan_kmod_dev_props *props = &device->kmod.dev->props;

   *unknown_gpu = false;
   device->model = pan_get_model(props->gpu_id, props->gpu_variant);
   if (device->model)
      return VK_SUCCESS;

   device->model = pan_get_fallback_model(props->gpu_id);
   if (!device->model) {
      return panvk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
                          "Unknown gpu_id (%#" PRIx64 ") or variant (%#x)",
                          props->gpu_id, props->gpu_variant);
   }

   *unknown_gpu = true;
   mesa_logw("panvk: unknown gpu_id (%#" PRIx64 ") or variant (%#x); "
             "continuing with conservative \"%s\" defaults — expect issues",
             props->gpu_id, props->gpu_variant, device->model->name);
   return VK_SUCCESS;
}

VkResult
panvk_physical_device_init(struct panvk_physical_device *device,
                           struct panvk_instance *instance,
                           drmDevicePtr drm_device)
{
   VkResult result;
   bool unknown_gpu;

   result = create_kmod_dev(device, instance, drm_device);
   if (result != VK_SUCCESS)
      return result;

   unsigned arch = pan_arch(device->kmod.dev->props.gpu_id);

   result = get_gpu_model(device, instance, &unknown_gpu);
   if (result != VK_SUCCESS)
      goto fail;

   switch (arch) {
   case 6:
   case 7:
   case 14:
      if (!os_get_option("PAN_I_WANT_A_BROKEN_VULKAN_DRIVER")) {
         result = panvk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
                               "WARNING: panvk is not well-tested on v%d, "
                               "pass PAN_I_WANT_A_BROKEN_VULKAN_DRIVER=1 "
                               "if you know what you're doing.", arch);
         goto fail;
      }
      break;

   case 10:
   case 12:
   case 13:
      break;

   default:
      result = panvk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
                            "%s not supported", device->model->name);
      goto fail;
   }

   result = get_drm_device_ids(device, instance, drm_device);
   if (result != VK_SUCCESS)
      goto fail;

   device->formats.all = pan_format_table(arch);
   device->formats.blendable = pan_blendable_format_table(arch);

   unsigned core_count =
      pan_query_core_count(&device->kmod.dev->props);

   memset(device->name, 0, sizeof(device->name));
   if (unknown_gpu)
      snprintf(device->name, sizeof(device->name),
               "Mali unknown 0x%" PRIx64 " MC%u",
               device->kmod.dev->props.gpu_id, core_count);
   else
      sprintf(device->name, "%s MC%u", device->model->name, core_count);

   result = get_core_masks(device, instance);
   if (result != VK_SUCCESS)
      goto fail;

   result = get_device_heaps(device, instance);
   if (result != VK_SUCCESS)
      goto fail;

   result = get_device_sync_types(device, instance);
   if (result != VK_SUCCESS)
      goto fail;

   if (arch >= 10) {
      /* XXX: Make dri options for thoses */
      device->csf.tiler.chunk_size = 2 * 1024 * 1024;
      device->csf.tiler.initial_chunks = 5;
      device->csf.tiler.max_chunks = 64;
   }

   if (arch != 10)
      vk_warn_non_conformant_implementation("panvk");

   struct vk_device_extension_table supported_extensions;
   panvk_arch_dispatch(arch, get_physical_device_extensions, device, instance,
                       &supported_extensions);

   struct vk_features supported_features;
   panvk_arch_dispatch(arch, get_physical_device_features, instance,
                       device, &supported_features);

   struct vk_physical_device_dispatch_table dispatch_table;
   vk_physical_device_dispatch_table_from_entrypoints(
      &dispatch_table, &panvk_physical_device_entrypoints, true);
   vk_physical_device_dispatch_table_from_entrypoints(
      &dispatch_table, &wsi_physical_device_entrypoints, false);

   result =
      vk_physical_device_init(&device->vk, &instance->vk, &supported_extensions,
                              &supported_features, NULL, &dispatch_table);

   if (result != VK_SUCCESS)
      goto fail;

   /* initialize disk cache after vk_physical_device_init */
   init_shader_caches(device, instance);

   /* pipeline binary props rely on disk cache init state */
   panvk_arch_dispatch(arch, get_physical_device_properties, instance, device,
                       &device->vk.properties);

   device->vk.supported_sync_types = device->sync_types;

   result = panvk_wsi_init(device);
   if (result != VK_SUCCESS)
      goto fail;

   return VK_SUCCESS;

fail:
   free_disk_cache(device);

   if (device->vk.instance)
      vk_physical_device_finish(&device->vk);

   pan_kmod_dev_destroy(device->kmod.dev);

   return result;
}

#if defined(HAVE_PAN_KMOD_KBASE)
VkResult
panvk_physical_device_init_kbase(struct panvk_physical_device *device,
                                 struct panvk_instance *instance,
                                 const char *path)
{
   VkResult result;

   result = create_kmod_dev_kbase(device, instance, path);
   if (result != VK_SUCCESS)
      return result;

   unsigned arch = pan_arch(device->kmod.dev->props.gpu_id);

   bool unknown_gpu;
   result = get_gpu_model(device, instance, &unknown_gpu);
   if (result != VK_SUCCESS)
      goto fail_kbase;

   switch (arch) {
   case 6:
   case 7:
   case 14:
      if (!os_get_option("PAN_I_WANT_A_BROKEN_VULKAN_DRIVER")) {
         result = panvk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
                               "WARNING: panvk is not well-tested on v%d, "
                               "pass PAN_I_WANT_A_BROKEN_VULKAN_DRIVER=1 "
                               "if you know what you're doing.", arch);
         goto fail_kbase;
      }
      break;

   case 10:
   case 11:
   case 12:
   case 13:
      break;

   default:
      result = panvk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
                            "%s not supported", device->model->name);
      goto fail_kbase;
   }

   /* kbase does not expose DRM nodes, so drm.{render,primary}_rdev are left
    * zeroed; WSI paths that compare rdev values will simply not match. */

   device->formats.all = pan_format_table(arch);
   device->formats.blendable = pan_blendable_format_table(arch);

   unsigned core_count =
      pan_query_core_count(&device->kmod.dev->props);

   memset(device->name, 0, sizeof(device->name));
   if (unknown_gpu)
      snprintf(device->name, sizeof(device->name),
               "Mali unknown 0x%" PRIx64 " MC%u",
               device->kmod.dev->props.gpu_id, core_count);
   else
      sprintf(device->name, "%s MC%u", device->model->name, core_count);

   result = get_core_masks(device, instance);
   if (result != VK_SUCCESS)
      goto fail_kbase;

   result = get_device_heaps(device, instance);
   if (result != VK_SUCCESS)
      goto fail_kbase;

   result = get_device_sync_types_kbase(device, instance);
   if (result != VK_SUCCESS)
      goto fail_kbase;

   if (arch >= 10) {
      /* 1 MiB chunks instead of the 2 MiB DRM-path default, as a
       * precaution against allocation failure under memory fragmentation:
       * kbase prefers order-9 huge pages for 2 MiB-aligned tiler-heap
       * chunks, and on a long-running Android system the order >= 4 free
       * lists are typically empty while gigabytes of order-0 pages remain.
       * 1 MiB chunks take the discontiguous small-page path, which only
       * needs order-0 pages; the GPU MMU provides the virtual contiguity.
       * Chunk counts are doubled to keep the same byte budget. */
      device->csf.tiler.chunk_size = 1024 * 1024;
      device->csf.tiler.initial_chunks = 10;
      device->csf.tiler.max_chunks = 400;
   }

   if (arch != 10)
      vk_warn_non_conformant_implementation("panvk");

   struct vk_device_extension_table supported_extensions;
   panvk_arch_dispatch(arch, get_physical_device_extensions, device, instance,
                       &supported_extensions);

   struct vk_features supported_features;
   panvk_arch_dispatch(arch, get_physical_device_features, instance,
                       device, &supported_features);

   struct vk_physical_device_dispatch_table dispatch_table;
   vk_physical_device_dispatch_table_from_entrypoints(
      &dispatch_table, &panvk_physical_device_entrypoints, true);
   vk_physical_device_dispatch_table_from_entrypoints(
      &dispatch_table, &wsi_physical_device_entrypoints, false);

   result =
      vk_physical_device_init(&device->vk, &instance->vk, &supported_extensions,
                              &supported_features, NULL, &dispatch_table);

   if (result != VK_SUCCESS)
      goto fail_kbase;

   init_shader_caches(device, instance);

   panvk_arch_dispatch(arch, get_physical_device_properties, instance, device,
                       &device->vk.properties);

   device->vk.supported_sync_types = device->sync_types;

   result = panvk_wsi_init(device);
   if (result != VK_SUCCESS)
      goto fail_kbase;

   return VK_SUCCESS;

fail_kbase:
   free_disk_cache(device);

   if (device->vk.instance)
      vk_physical_device_finish(&device->vk);

   pan_kmod_dev_destroy(device->kmod.dev);

   return result;
}
#endif /* HAVE_PAN_KMOD_KBASE */

static void
panvk_fill_global_priority(const struct panvk_physical_device *physical_device,
                           uint32_t family_idx,
                           VkQueueFamilyGlobalPriorityPropertiesKHR *prio)
{
   const unsigned arch = pan_arch(physical_device->kmod.dev->props.gpu_id);
   uint32_t prio_idx = 0;

   switch (family_idx) {
   case PANVK_QUEUE_FAMILY_GPU: {
      enum pan_kmod_group_allow_priority_flags prio_mask =
         physical_device->kmod.dev->props.allowed_group_priorities_mask;

      /* Non-medium priority context is not hooked-up in the JM backend, even
       * though the panfrost kmod advertize it. Manually filter non-medium
       * priority for now.
       */
      if (arch < 10)
         prio_mask &= PAN_KMOD_GROUP_ALLOW_PRIORITY_MEDIUM;

      if (prio_mask & PAN_KMOD_GROUP_ALLOW_PRIORITY_LOW)
         prio->priorities[prio_idx++] = VK_QUEUE_GLOBAL_PRIORITY_LOW_KHR;
      if (prio_mask & PAN_KMOD_GROUP_ALLOW_PRIORITY_MEDIUM)
         prio->priorities[prio_idx++] = VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_KHR;
      if (prio_mask & PAN_KMOD_GROUP_ALLOW_PRIORITY_HIGH)
         prio->priorities[prio_idx++] = VK_QUEUE_GLOBAL_PRIORITY_HIGH_KHR;
      if (prio_mask & PAN_KMOD_GROUP_ALLOW_PRIORITY_REALTIME)
         prio->priorities[prio_idx++] = VK_QUEUE_GLOBAL_PRIORITY_REALTIME_KHR;
      break;
   }

   case PANVK_QUEUE_FAMILY_BIND:
      prio->priorities[prio_idx++] = VK_QUEUE_GLOBAL_PRIORITY_MEDIUM;
      break;

   default:
      UNREACHABLE("Unknown queue family");
   }

   prio->priorityCount = prio_idx;
}

VKAPI_ATTR void VKAPI_CALL
panvk_GetPhysicalDeviceQueueFamilyProperties2(
   VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount,
   VkQueueFamilyProperties2 *pQueueFamilyProperties)
{
   VK_FROM_HANDLE(panvk_physical_device, physical_device, physicalDevice);
   VK_OUTARRAY_MAKE_TYPED(VkQueueFamilyProperties2, out, pQueueFamilyProperties,
                          pQueueFamilyPropertyCount);
   unsigned arch = pan_arch(physical_device->kmod.dev->props.gpu_id);

   const VkQueueFamilyProperties qfamily_props[PANVK_QUEUE_FAMILY_COUNT] = {
      [PANVK_QUEUE_FAMILY_GPU] = {
         .queueFlags =
            VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT |
            (physical_device->vk.supported_features.sparseBinding
                ? VK_QUEUE_SPARSE_BINDING_BIT
                : 0),
         /* On v10+ we can support up to 127 queues but this causes timeout on
            some CTS tests */
         .queueCount = arch >= 10 ? 2 : 1,
         .timestampValidBits =
            arch >= 10 &&
                  physical_device->kmod.dev->props.gpu_can_query_timestamp
               ? 64
               : 0,
         .minImageTransferGranularity = {1, 1, 1},
      },
      [PANVK_QUEUE_FAMILY_BIND] = {
         .queueFlags = VK_QUEUE_SPARSE_BINDING_BIT,
         .queueCount = 1,
      },
   };

   for (uint32_t family = 0; family < ARRAY_SIZE(qfamily_props); family++) {
      if (family == PANVK_QUEUE_FAMILY_BIND &&
          !physical_device->vk.supported_features.sparseBinding)
         break;

      vk_outarray_append_typed(VkQueueFamilyProperties2, &out, p) {
         p->queueFamilyProperties = qfamily_props[family];

         VkQueueFamilyGlobalPriorityPropertiesKHR *prio =
            vk_find_struct(p->pNext, QUEUE_FAMILY_GLOBAL_PRIORITY_PROPERTIES_KHR);
         if (prio)
            panvk_fill_global_priority(physical_device, family, prio);
      }
   }
}

VKAPI_ATTR void VKAPI_CALL
panvk_GetPhysicalDeviceMemoryProperties2(
   VkPhysicalDevice physicalDevice,
   VkPhysicalDeviceMemoryProperties2 *pMemoryProperties)
{
   VK_FROM_HANDLE(panvk_physical_device, physical_device, physicalDevice);

   pMemoryProperties->memoryProperties.memoryHeapCount =
      physical_device->memory.heap_count;
   for (uint32_t i = 0; i < physical_device->memory.heap_count; i++) {
      pMemoryProperties->memoryProperties.memoryHeaps[i] =
          physical_device->memory.heaps[i];
   }

   pMemoryProperties->memoryProperties.memoryTypeCount =
      physical_device->memory.type_count;
   for (uint32_t i = 0; i < physical_device->memory.type_count; i++) {
      pMemoryProperties->memoryProperties.memoryTypes[i] =
          physical_device->memory.types[i];
   }

   vk_foreach_struct(ext, pMemoryProperties->pNext) {
      switch (ext->sType) {
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT: {
         VkPhysicalDeviceMemoryBudgetPropertiesEXT *p = (void *)ext;

         uint64_t used = p_atomic_read(&physical_device->memory.heap_used);
         uint64_t heap_size = physical_device->memory.heaps[0].size;

         /* From the Vulkan 1.3.278 spec:
          *
          *    "heapUsage is an array of VK_MAX_MEMORY_HEAPS VkDeviceSize
          *    values in which memory usages are returned, with one element
          *    for each memory heap. A heap’s usage is an estimate of how
          *    much memory the process is currently using in that heap."
          */
         p->heapUsage[0] = used;

         /* Set the budget at 90% of available to avoid thrashing */
         p->heapBudget[0] = vk_physical_device_heap_budget_from_system(
            &physical_device->vk, 0.9f, heap_size, used);

         /* From the Vulkan 1.3.278 spec:
          *
          *    "The heapBudget and heapUsage values must be zero for array
          *    elements greater than or equal to
          *    VkPhysicalDeviceMemoryProperties::memoryHeapCount. The
          *    heapBudget value must be non-zero for array elements less than
          *    VkPhysicalDeviceMemoryProperties::memoryHeapCount."
          */
         for (unsigned i = 1; i < VK_MAX_MEMORY_HEAPS; i++) {
            p->heapBudget[i] = 0;
            p->heapUsage[i] = 0;
         }
         break;
      }
      default:
         vk_debug_ignored_stype(ext->sType);
         break;
      }
   }
}

VKAPI_ATTR VkResult VKAPI_CALL
panvk_CreateDevice(VkPhysicalDevice physicalDevice,
                   const VkDeviceCreateInfo *pCreateInfo,
                   const VkAllocationCallbacks *pAllocator, VkDevice *pDevice)
{
   VK_FROM_HANDLE(panvk_physical_device, physical_device, physicalDevice);
   unsigned arch = pan_arch(physical_device->kmod.dev->props.gpu_id);
   VkResult result = VK_ERROR_INITIALIZATION_FAILED;

   panvk_arch_dispatch_ret(arch, create_device, result, physical_device,
                           pCreateInfo, pAllocator, pDevice);

   return result;
}

VKAPI_ATTR void VKAPI_CALL
panvk_DestroyDevice(VkDevice _device, const VkAllocationCallbacks *pAllocator)
{
   VK_FROM_HANDLE(panvk_device, device, _device);
   struct panvk_physical_device *physical_device =
      to_panvk_physical_device(device->vk.physical);
   unsigned arch = pan_arch(physical_device->kmod.dev->props.gpu_id);

   panvk_arch_dispatch(arch, destroy_device, device, pAllocator);
}

static bool
unsupported_yuv_format(enum pipe_format pfmt)
{
   switch (pfmt) {
   /* 3-plane YUV 444 and 16-bit 3-plane YUV are not supported natively by
    * the HW.
    */
   case PIPE_FORMAT_Y8_U8_V8_444_UNORM:
   case PIPE_FORMAT_Y16_U16_V16_420_UNORM:
   case PIPE_FORMAT_Y16_U16_V16_422_UNORM:
   case PIPE_FORMAT_Y16_U16_V16_444_UNORM:
      return true;
   default:
      return false;
   }
}

static bool
format_is_supported(struct panvk_physical_device *physical_device,
                    const struct pan_format fmt, enum pipe_format pfmt)
{
   if (pfmt == PIPE_FORMAT_NONE)
      return false;

   if (unsupported_yuv_format(pfmt))
      return false;

   /* If the format ID is zero, it's not supported. */
   if (!fmt.hw)
      return false;

   /* Compressed formats (ID < 32) are optional. We need to check against
    * the supported formats reported by the GPU. */
   if (util_format_is_compressed(pfmt)) {
      uint32_t supported_compr_fmts =
         pan_query_compressed_formats(&physical_device->kmod.dev->props);

      if (!(BITFIELD_BIT(fmt.texfeat_bit) & supported_compr_fmts))
         return false;
   }

   return true;
}

static VkFormatFeatureFlags2
get_image_plane_format_features(struct panvk_physical_device *physical_device,
                                VkFormat format)
{
   VkFormatFeatureFlags2 features = 0;
   enum pipe_format pfmt = vk_format_to_pipe_format(format);
   const struct pan_format fmt = physical_device->formats.all[pfmt];
   unsigned arch = pan_arch(physical_device->kmod.dev->props.gpu_id);

   if (!format_is_supported(physical_device, fmt, pfmt))
      return 0;

   if (fmt.bind & PAN_BIND_SAMPLER_VIEW) {
      features |= VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT |
                  VK_FORMAT_FEATURE_2_TRANSFER_DST_BIT |
                  VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT;

      if (arch >= 10)
         features |= VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_MINMAX_BIT;

      /* Integer formats only support nearest filtering */
      if (!util_format_is_scaled(pfmt) && !util_format_is_pure_integer(pfmt))
         features |= VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

      features |= VK_FORMAT_FEATURE_2_BLIT_SRC_BIT;

      if (vk_format_has_depth(format))
         features |= VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_DEPTH_COMPARISON_BIT;
   }

   if (fmt.bind & PAN_BIND_RENDER_TARGET) {
      features |= VK_FORMAT_FEATURE_2_BLIT_DST_BIT;
      features |= VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT;
      features |= VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BLEND_BIT;
   }

   const bool is_r64 = util_format_is_int64(util_format_description(pfmt));

   if (fmt.bind & PAN_BIND_STORAGE_IMAGE) {
      features |= VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT;

      /* R64 does not support formatless access. */
      if (!is_r64)
         features |= VK_FORMAT_FEATURE_2_STORAGE_READ_WITHOUT_FORMAT_BIT |
                     VK_FORMAT_FEATURE_2_STORAGE_WRITE_WITHOUT_FORMAT_BIT;

      if (pfmt == PIPE_FORMAT_R32_UINT || pfmt == PIPE_FORMAT_R32_SINT ||
          pfmt == PIPE_FORMAT_R32_FLOAT || is_r64)
         features |= VK_FORMAT_FEATURE_2_STORAGE_IMAGE_ATOMIC_BIT;
   }

   /* R64 lacks SAMPLER_VIEW - grant transfer bits for host-visible readback. */
   if (is_r64 && (fmt.bind & PAN_BIND_STORAGE_IMAGE))
      features |= VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT |
                  VK_FORMAT_FEATURE_2_TRANSFER_DST_BIT;

   if (fmt.bind & PAN_BIND_DEPTH_STENCIL)
      features |= VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT;

   if (features != 0)
      features |= VK_FORMAT_FEATURE_2_HOST_IMAGE_TRANSFER_BIT;

   return features;
}

static VkFormatFeatureFlags2
get_image_format_features(struct panvk_physical_device *physical_device,
                          VkFormat format)
{
   const struct vk_format_ycbcr_info *ycbcr_info =
         vk_format_get_ycbcr_info(format);

   if (ycbcr_info == NULL)
      return get_image_plane_format_features(physical_device, format);

   if (unsupported_yuv_format(vk_format_to_pipe_format(format)))
      return 0;

   /* For multi-plane, we get the feature flags of each plane separately,
    * then take their intersection as the overall format feature flags
    */
   VkFormatFeatureFlags2 features = ~0ull;
   bool cosited_chroma = false;
   for (uint8_t plane = 0; plane < ycbcr_info->n_planes; plane++) {
      const struct vk_format_ycbcr_plane *plane_info =
         &ycbcr_info->planes[plane];
      features &=
         get_image_plane_format_features(physical_device, plane_info->format);
      if (plane_info->denominator_scales[0] > 1 ||
          plane_info->denominator_scales[1] > 1)
         cosited_chroma = true;
   }
   if (features == 0)
      return 0;

   /* Uh... We really should be able to sample from YCbCr */
   assert(features & VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT);
   assert(features & VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

   /* Siting is handled in the YCbCr lowering pass. */
   features |= VK_FORMAT_FEATURE_2_MIDPOINT_CHROMA_SAMPLES_BIT;
   if (cosited_chroma)
      features |= VK_FORMAT_FEATURE_2_COSITED_CHROMA_SAMPLES_BIT;

   /* These aren't allowed for YCbCr formats */
   features &= ~(VK_FORMAT_FEATURE_2_BLIT_SRC_BIT |
                 VK_FORMAT_FEATURE_2_BLIT_DST_BIT |
                 VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT |
                 VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BLEND_BIT |
                 VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT |
                 VK_FORMAT_FEATURE_2_STORAGE_READ_WITHOUT_FORMAT_BIT |
                 VK_FORMAT_FEATURE_2_STORAGE_WRITE_WITHOUT_FORMAT_BIT);

   /* This is supported on all YCbCr formats */
   features |=
      VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT;

   if (ycbcr_info->n_planes > 1) {
      /* DISJOINT_BIT implies that each plane has its own separate binding,
       * while SEPARATE_RECONSTRUCTION_FILTER_BIT implies that luma and chroma
       * each have their own, separate filters, so these two bits make sense
       * for multi-planar formats only.
       */
      features |= VK_FORMAT_FEATURE_2_DISJOINT_BIT;

      /* YUV texturing only support unified filtering across planes. */
      unsigned arch = pan_arch(physical_device->kmod.dev->props.gpu_id);
      if (!panvk_image_use_yuv_tex(arch, format)) {
         features |=
            VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT;
      }
   }

   return features;
}

/* Note: update nir_shader_compiler_options.max_samples when changing this. */
VkSampleCountFlags
panvk_get_sample_counts(unsigned arch, unsigned max_tib_size,
                        unsigned max_cbuf_atts, unsigned format_size)
{
   VkSampleCountFlags sample_counts =
      VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_4_BIT;

   unsigned max_msaa =
      pan_get_max_msaa(arch, max_tib_size, max_cbuf_atts, format_size);

   assert(max_msaa >= 4);

   if (arch >= 12)
      sample_counts |= VK_SAMPLE_COUNT_2_BIT;

   if (max_msaa >= 8)
      sample_counts |= VK_SAMPLE_COUNT_8_BIT;

   if (max_msaa >= 16)
      sample_counts |= VK_SAMPLE_COUNT_16_BIT;

   return sample_counts;
}

/* Plane descriptors have limits to how large resources they can encode, both
 * for the buffer size and for the slice-strides.
 *
 * The only mandates we have from the Vulkan spec to limit resource sizes,
 * is to use the maxImageDimension* limits, or through maxResourceSize.
 * Limiting using maxImageDimension* has application compatibility problems,
 * so let's use maxResourceSize.
 *
 * Unfortunately, this means we have to limit the *entire* resource to the
 * limit, rather than just a single image plane.
 */

VkDeviceSize
panvk_get_max_resource_size(const struct panvk_physical_device *device)
{
   const unsigned arch = pan_arch(device->kmod.dev->props.gpu_id);
   unsigned max_desc_size = u_uintN_max(arch < 11 ? 32 : 48);
   return MIN2(max_desc_size, device->memory.max_supported_va);
}

VkDeviceSize
panvk_get_max_buffer_size(const struct panvk_physical_device *device)
{
   const unsigned arch = pan_arch(device->kmod.dev->props.gpu_id);
   unsigned max_desc_size = u_uintN_max(arch < 11 ? 32 : 48);
   return MIN2(max_desc_size, device->memory.max_supported_va);
}

static VkFormatFeatureFlags2
get_image_format_sample_counts(struct panvk_physical_device *physical_device,
                               VkFormat format)
{
   unsigned arch = pan_arch(physical_device->kmod.dev->props.gpu_id);
   unsigned max_tib_size = pan_query_tib_size(physical_device->model);
   unsigned max_cbuf_atts = pan_get_max_cbufs(arch, max_tib_size);

   assert(!vk_format_is_compressed(format));

   enum pipe_format pfmt = vk_format_to_pipe_format(format);
   unsigned format_size =
      pan_format_tib_size(pfmt, physical_device->formats.blendable[pfmt].internal);

   return panvk_get_sample_counts(arch, max_tib_size, max_cbuf_atts,
                                  format_size);
}

static VkFormatFeatureFlags2
get_buffer_format_features(struct panvk_physical_device *physical_device,
                           VkFormat format)
{
   VkFormatFeatureFlags2 features = 0;
   enum pipe_format pfmt = vk_format_to_pipe_format(format);
   const struct pan_format fmt = physical_device->formats.all[pfmt];

   if (!format_is_supported(physical_device, fmt, pfmt))
      return 0;

   /* Reject sRGB formats (see
    * https://github.com/KhronosGroup/Vulkan-Docs/issues/2214).
    */
   if ((fmt.bind & PAN_BIND_VERTEX_BUFFER) && !util_format_is_srgb(pfmt))
      features |= VK_FORMAT_FEATURE_2_VERTEX_BUFFER_BIT;

   if (fmt.bind & PAN_BIND_TEXEL_BUFFER)
      features |= VK_FORMAT_FEATURE_2_UNIFORM_TEXEL_BUFFER_BIT |
                  VK_FORMAT_FEATURE_2_STORAGE_TEXEL_BUFFER_BIT |
                  VK_FORMAT_FEATURE_2_STORAGE_READ_WITHOUT_FORMAT_BIT |
                  VK_FORMAT_FEATURE_2_STORAGE_WRITE_WITHOUT_FORMAT_BIT;

   if (pfmt == PIPE_FORMAT_R32_UINT || pfmt == PIPE_FORMAT_R32_SINT)
      features |= VK_FORMAT_FEATURE_2_STORAGE_TEXEL_BUFFER_ATOMIC_BIT;

   return features;
}

VKAPI_ATTR void VKAPI_CALL
panvk_GetPhysicalDeviceFormatProperties2(VkPhysicalDevice physicalDevice,
                                         VkFormat format,
                                         VkFormatProperties2 *pFormatProperties)
{
   VK_FROM_HANDLE(panvk_physical_device, physical_device, physicalDevice);
   const unsigned arch = pan_arch(physical_device->kmod.dev->props.gpu_id);

   VkFormatFeatureFlags2 tex =
      get_image_format_features(physical_device, format);
   VkFormatFeatureFlags2 buffer =
      get_buffer_format_features(physical_device, format);

   VkFormatFeatureFlags tex_legacy = vk_format_features2_to_features(tex);
   VkFormatFeatureFlags buffer_legacy =
      vk_format_features2_to_features(buffer);

   pFormatProperties->formatProperties = (VkFormatProperties){
      .linearTilingFeatures = tex_legacy,
      .optimalTilingFeatures = tex_legacy,
      .bufferFeatures = buffer_legacy,
   };

   VkFormatProperties3 *formatProperties3 =
      vk_find_struct(pFormatProperties->pNext, FORMAT_PROPERTIES_3);
   if (formatProperties3) {
      formatProperties3->linearTilingFeatures = tex;
      formatProperties3->optimalTilingFeatures = tex;
      formatProperties3->bufferFeatures = buffer;
   }

   const uint32_t plane_count = vk_format_get_plane_count(format);

   PAN_SUPPORTED_MODIFIERS(supported);
   uint64_t afbc_modifiers[ARRAY_SIZE(supported)];
   uint32_t afbc_modifier_count = 0;
   if (PANVK_DEBUG(WSI_AFBC) &&
         pan_afbc_supports_format(arch, vk_format_to_pipe_format(format))) {
      for (uint32_t mi = 0; mi < ARRAY_SIZE(supported); mi++) {
         if (drm_is_afbc(supported[mi]))
            afbc_modifiers[afbc_modifier_count++] = supported[mi];
      }
   }
   VkDrmFormatModifierPropertiesListEXT *list = vk_find_struct(
      pFormatProperties->pNext, DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT);
   if (list) {
      VkFormatFeatureFlags optimal_features =
         pFormatProperties->formatProperties.optimalTilingFeatures;
      VkFormatFeatureFlags linear_features =
         pFormatProperties->formatProperties.linearTilingFeatures;

      VK_OUTARRAY_MAKE_TYPED(VkDrmFormatModifierPropertiesEXT, out,
                              list->pDrmFormatModifierProperties,
                              &list->drmFormatModifierCount);

      if (optimal_features) {
         /* Multi-planar AFBC is not supported. */
         assert(!afbc_modifier_count || plane_count == 1);

         for (uint32_t i = 0; i < afbc_modifier_count; i++) {
            vk_outarray_append_typed(VkDrmFormatModifierPropertiesEXT, &out,
                                       mod_props)
            {
               mod_props->drmFormatModifier = afbc_modifiers[i];
               mod_props->drmFormatModifierPlaneCount = plane_count;
               mod_props->drmFormatModifierTilingFeatures = optimal_features;
            }
         }
      }

      if (linear_features) {
         vk_outarray_append_typed(VkDrmFormatModifierPropertiesEXT, &out,
                                    mod_props)
         {
            mod_props->drmFormatModifier = DRM_FORMAT_MOD_LINEAR;
            mod_props->drmFormatModifierPlaneCount = plane_count;
            mod_props->drmFormatModifierTilingFeatures = linear_features;
         }
      }
   }
   VkDrmFormatModifierPropertiesList2EXT *list2 = vk_find_struct(
      pFormatProperties->pNext, DRM_FORMAT_MODIFIER_PROPERTIES_LIST_2_EXT);
   if (list2) {
      VkFormatFeatureFlags2 optimal_features2 = tex;
      VkFormatFeatureFlags2 linear_features2 = tex;

      VK_OUTARRAY_MAKE_TYPED(VkDrmFormatModifierProperties2EXT, out,
                              list2->pDrmFormatModifierProperties,
                              &list2->drmFormatModifierCount);

      if (optimal_features2) {
         /* Multi-planar AFBC is not supported. */
         assert(!afbc_modifier_count || plane_count == 1);

         for (uint32_t i = 0; i < afbc_modifier_count; i++) {
            vk_outarray_append_typed(VkDrmFormatModifierProperties2EXT, &out,
                                       mod_props)
            {
               mod_props->drmFormatModifier = afbc_modifiers[i];
               mod_props->drmFormatModifierPlaneCount = plane_count;
               mod_props->drmFormatModifierTilingFeatures =
                  optimal_features2;
            }
         }
      }

      if (linear_features2) {
         vk_outarray_append_typed(VkDrmFormatModifierProperties2EXT, &out,
                                    mod_props)
         {
            mod_props->drmFormatModifier = DRM_FORMAT_MOD_LINEAR;
            mod_props->drmFormatModifierPlaneCount = plane_count;
            mod_props->drmFormatModifierTilingFeatures = linear_features2;
         }
      }
   }

   VkSubpassResolvePerformanceQueryEXT *subpass_resolve_perf = vk_find_struct(
      pFormatProperties->pNext, SUBPASS_RESOLVE_PERFORMANCE_QUERY_EXT);
   if (subpass_resolve_perf) {
      /* We always resolve in a separate command instead of in HW atm. */
      subpass_resolve_perf->optimal = VK_FALSE;
   }
}

static VkResult
get_image_format_properties(struct panvk_physical_device *physical_device,
                            const VkPhysicalDeviceImageFormatInfo2 *info,
                            VkImageFormatProperties *pImageFormatProperties,
                            VkFormatFeatureFlags2 *p_feature_flags)
{
   VkFormatFeatureFlags2 format_feature_flags;
   VkExtent3D maxExtent;
   uint32_t maxMipLevels;
   uint32_t maxArraySize;
   VkSampleCountFlags sampleCounts = VK_SAMPLE_COUNT_1_BIT;
   enum pipe_format format = vk_format_to_pipe_format(info->format);

   const VkImageStencilUsageCreateInfo *stencil_usage_info =
      vk_find_struct_const(info->pNext, IMAGE_STENCIL_USAGE_CREATE_INFO);
   VkImageUsageFlags stencil_usage =
      stencil_usage_info ? stencil_usage_info->stencilUsage : info->usage;
   VkImageUsageFlags all_usage = info->usage | stencil_usage;
   const struct vk_format_ycbcr_info *ycbcr_info =
      vk_format_get_ycbcr_info(info->format);

   if (info->flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT) {
      if (!physical_device->vk.supported_features.sparseBinding)
         goto unsupported;

      /*
       * Sparse only manipulates device mappings and we implement host copies on
       * host. Purely hypotetically, we could implement host copies for sparse
       * images in one of, but not limited to, the following ways:
       *
       *    * submitting a device copy and immediately waiting on it
       *
       *    * mirror sparse binds' modifications to device mappings on host
       *
       *    * share a single address space and thus mappings between host and device
       *
       * but realistically speaking, the set of people, apps and tests in the
       * CTS that expect a driver to implement host copies on sparse images is
       * exactly empty, so let's just not bother.
       */
      if (info->usage & VK_IMAGE_USAGE_HOST_TRANSFER_BIT)
         goto unsupported;
   }

   if (info->flags & VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT) {
      if (!((info->type == VK_IMAGE_TYPE_2D &&
             physical_device->vk.supported_features.sparseResidencyImage2D) ||
            (info->type == VK_IMAGE_TYPE_3D &&
             physical_device->vk.supported_features.sparseResidencyImage3D)))
         goto unsupported;

      /* Only single aspect (thus single plane) stuff is supported for now */
      if (util_bitcount(vk_format_aspects(info->format)) != 1)
         goto unsupported;

      if (info->tiling != VK_IMAGE_TILING_OPTIMAL)
         goto unsupported;

      struct panvk_sparse_block_desc sblock_desc = panvk_get_sparse_block_desc(info->type, info->format);
      if (!panvk_sparse_block_is_valid(sblock_desc))
         goto unsupported;

      VkImageUsageFlags allowed_usages =
         VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
         VK_IMAGE_USAGE_TRANSFER_DST_BIT |
         VK_IMAGE_USAGE_SAMPLED_BIT |
         VK_IMAGE_USAGE_STORAGE_BIT |
         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
         VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
      if (all_usage & ~allowed_usages)
         goto unsupported;
   }

   switch (info->tiling) {
   case VK_IMAGE_TILING_LINEAR:
   case VK_IMAGE_TILING_OPTIMAL:
      break;
   case VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT: {
      const VkPhysicalDeviceImageDrmFormatModifierInfoEXT *mod_info =
         vk_find_struct_const(
            info->pNext, PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT);

      /* TODO: switch to using a more generic function for checking mod support here
       * when adding new modifiers, so that this case doesn't become too big. */
      const bool can_use_afbc =
         PANVK_DEBUG(WSI_AFBC) &&
         panvk_image_can_use_afbc(physical_device, info->format, info->usage,
                                  info->type, info->tiling, 0);
      const bool supported = (drm_is_afbc(mod_info->drmFormatModifier) && can_use_afbc) ||
         mod_info->drmFormatModifier == DRM_FORMAT_MOD_LINEAR;
      if (!supported)
         goto unsupported;

      /* The only difference between optimal and linear is currently whether
       * depth/stencil attachments are allowed on depth/stencil formats.
       * There's no reason to allow importing depth/stencil textures, so just
       * disallow it and then this annoying edge case goes away.
       */
      if (util_format_is_depth_or_stencil(format))
         goto unsupported;
      break;
   }
   default:
      /* VK_KHR_maintenance5: Physical-device-level functions can now be called
       * with any value in the valid range for a type beyond the defined
       * enumerants [...] */
      goto unsupported;
   }

   /* For the purposes of these checks, we don't care about all the extra
    * YCbCr features and we just want the intersection of features available
    * to all planes of the given format.
    */
   if (ycbcr_info == NULL) {
      format_feature_flags =
         get_image_format_features(physical_device, info->format);
   } else {
      format_feature_flags = ~0u;
      assert(ycbcr_info->n_planes > 0);
      for (uint8_t plane = 0; plane < ycbcr_info->n_planes; plane++) {
         const VkFormat plane_format = ycbcr_info->planes[plane].format;
         format_feature_flags &=
            get_image_format_features(physical_device, plane_format);
      }
   }

   if (format_feature_flags == 0)
      goto unsupported;

   if (ycbcr_info && info->type != VK_IMAGE_TYPE_2D)
      goto unsupported;

   switch (info->type) {
   case VK_IMAGE_TYPE_1D:
      maxExtent.width = 1 << 16;
      maxExtent.height = 1;
      maxExtent.depth = 1;
      maxMipLevels = 17; /* log2(maxWidth) + 1 */
      maxArraySize = 1 << 16;
      break;
   case VK_IMAGE_TYPE_2D:
      maxExtent.width = 1 << 16;
      maxExtent.height = 1 << 16;
      maxExtent.depth = 1;
      maxMipLevels = 17; /* log2(maxWidth) + 1 */
      maxArraySize = 1 << 16;
      break;
   case VK_IMAGE_TYPE_3D:
      maxExtent.width = 1 << 16;
      maxExtent.height = 1 << 16;
      maxExtent.depth = 1 << 16;
      maxMipLevels = 17; /* log2(maxWidth) + 1 */
      maxArraySize = 1;
      break;
   default:
      /* VK_KHR_maintenance5: Physical-device-level functions can now be called
       * with any value in the valid range for a type beyond the defined
       * enumerants [...] */
      goto unsupported;
   }

   if (ycbcr_info)
      maxMipLevels = 1;

   if (info->tiling == VK_IMAGE_TILING_OPTIMAL &&
       info->type == VK_IMAGE_TYPE_2D && ycbcr_info == NULL &&
       (format_feature_flags &
        (VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT |
         VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT)) &&
       !(info->flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) &&
       !(all_usage & VK_IMAGE_USAGE_STORAGE_BIT)) {
      sampleCounts |=
         get_image_format_sample_counts(physical_device, info->format);
   }

   /* From the Vulkan 1.2.199 spec:
   *
   *    "VK_IMAGE_CREATE_EXTENDED_USAGE_BIT specifies that the image can be
   *    created with usage flags that are not supported for the format the
   *    image is created with but are supported for at least one format a
   *    VkImageView created from the image can have."
   *
   * If VK_IMAGE_CREATE_EXTENDED_USAGE_BIT is set, views can be created with
   * different usage than the image so we can't always filter on usage.
   * There is one exception to this below for storage.
   */
   if (!(info->flags & VK_IMAGE_CREATE_EXTENDED_USAGE_BIT)) {
      if (all_usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
         if (!(format_feature_flags & VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT)) {
            goto unsupported;
         }
      }

      if (all_usage & VK_IMAGE_USAGE_STORAGE_BIT) {
         if (!(format_feature_flags & VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT)) {
            goto unsupported;
         }
      }

      if (all_usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
          ((all_usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) &&
           !vk_format_is_depth_or_stencil(info->format))) {
         if (!(format_feature_flags & VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT)) {
            goto unsupported;
         }
      }

      if ((all_usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ||
          ((all_usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) &&
           vk_format_is_depth_or_stencil(info->format))) {
         if (!(format_feature_flags &
               VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT)) {
            goto unsupported;
         }
      }
   }

   *pImageFormatProperties = (VkImageFormatProperties){
      .maxExtent = maxExtent,
      .maxMipLevels = maxMipLevels,
      .maxArrayLayers = maxArraySize,
      .sampleCounts = sampleCounts,
      .maxResourceSize = panvk_get_max_resource_size(physical_device),
   };

   if (p_feature_flags)
      *p_feature_flags = format_feature_flags;

   return VK_SUCCESS;
unsupported:
   *pImageFormatProperties = (VkImageFormatProperties){
      .maxExtent = {0, 0, 0},
      .maxMipLevels = 0,
      .maxArrayLayers = 0,
      .sampleCounts = 0,
      .maxResourceSize = 0,
   };

   return VK_ERROR_FORMAT_NOT_SUPPORTED;
}

static VkResult
panvk_get_external_image_format_properties(
   const struct panvk_physical_device *physical_device,
   const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo,
   VkExternalMemoryHandleTypeFlagBits handleType,
   VkExternalMemoryProperties *external_properties)
{
   const VkExternalMemoryHandleTypeFlags supported_handle_types =
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT |
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

   if (!(handleType & supported_handle_types)) {
      return panvk_errorf(physical_device, VK_ERROR_FORMAT_NOT_SUPPORTED,
                          "VkExternalMemoryTypeFlagBits(0x%x) unsupported",
                          handleType);
   }

   /* pan_image_layout_init requires 2D for explicit layout */
   if (pImageFormatInfo->type != VK_IMAGE_TYPE_2D) {
      return panvk_errorf(
         physical_device, VK_ERROR_FORMAT_NOT_SUPPORTED,
         "VkExternalMemoryTypeFlagBits(0x%x) unsupported for VkImageType(%d)",
         handleType, pImageFormatInfo->type);
   }

   /* There is no restriction on opaque fds.  But for dma-bufs, we want to
    * make sure vkGetImageSubresourceLayout can be used to query the image
    * layout of an exported dma-buf.  We also want to make sure
    * VkImageDrmFormatModifierExplicitCreateInfoEXT can be used to specify the
    * image layout of an imported dma-buf.  These add restrictions on the
    * image tilings.
    */
   VkExternalMemoryFeatureFlags features = 0;
   if (handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT ||
       pImageFormatInfo->tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
      features |= VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT |
                  VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT;
   } else if (pImageFormatInfo->tiling == VK_IMAGE_TILING_LINEAR) {
      features |= VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT;
   }

   if (!features) {
      return panvk_errorf(
         physical_device, VK_ERROR_FORMAT_NOT_SUPPORTED,
         "VkExternalMemoryTypeFlagBits(0x%x) unsupported for VkImageTiling(%d)",
         handleType, pImageFormatInfo->tiling);
   }

   *external_properties = (VkExternalMemoryProperties){
      .externalMemoryFeatures = features,
      .exportFromImportedHandleTypes = supported_handle_types,
      .compatibleHandleTypes = supported_handle_types,
   };

   return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
panvk_GetPhysicalDeviceImageFormatProperties2(
   VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceImageFormatInfo2 *base_info,
   VkImageFormatProperties2 *base_props)
{
   VK_FROM_HANDLE(panvk_physical_device, physical_device, physicalDevice);
   const VkImageStencilUsageCreateInfo *stencil_usage_info = NULL;
   const VkPhysicalDeviceExternalImageFormatInfo *external_info = NULL;
   const VkPhysicalDeviceImageViewImageFormatInfoEXT *image_view_info = NULL;
   VkExternalImageFormatProperties *external_props = NULL;
   VkFilterCubicImageViewImageFormatPropertiesEXT *cubic_props = NULL;
   VkFormatFeatureFlags2 format_feature_flags;
   VkHostImageCopyDevicePerformanceQuery *hic_props = NULL;
   VkSamplerYcbcrConversionImageFormatProperties *ycbcr_props = NULL;
   VkResult result;

   result = get_image_format_properties(physical_device, base_info,
                                        &base_props->imageFormatProperties,
                                        &format_feature_flags);
   if (result != VK_SUCCESS)
      return result;

   /* Extract input structs */
   vk_foreach_struct_const(s, base_info->pNext) {
      switch (s->sType) {
      case VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO:
         stencil_usage_info = (const void*)s;
         break;
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO:
         external_info = (const void *)s;
         break;
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_IMAGE_FORMAT_INFO_EXT:
         image_view_info = (const void *)s;
         break;
      default:
         break;
      }
   }

   /* Extract output structs */
   vk_foreach_struct(s, base_props->pNext) {
      switch (s->sType) {
      case VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES:
         external_props = (void *)s;
         break;
      case VK_STRUCTURE_TYPE_FILTER_CUBIC_IMAGE_VIEW_IMAGE_FORMAT_PROPERTIES_EXT:
         cubic_props = (void *)s;
         break;
      case VK_STRUCTURE_TYPE_HOST_IMAGE_COPY_DEVICE_PERFORMANCE_QUERY:
         hic_props = (void *)s;
         break;
      case VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES:
         ycbcr_props = (void *)s;
         break;
      default:
         break;
      }
   }

   /* From the Vulkan 1.0.42 spec:
    *
    *    If handleType is 0, vkGetPhysicalDeviceImageFormatProperties2 will
    *    behave as if VkPhysicalDeviceExternalImageFormatInfo was not
    *    present and VkExternalImageFormatProperties will be ignored.
    */
   if (external_info && external_info->handleType != 0) {
      VkExternalImageFormatProperties fallback_external_props;

      if (!external_props) {
         memset(&fallback_external_props, 0, sizeof(fallback_external_props));
         external_props = &fallback_external_props;
      }

      if (external_info->handleType ==
          VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID) {
         result = vk_android_get_ahb_image_properties(physicalDevice, base_info,
                                                      base_props);
      } else {
         result = panvk_get_external_image_format_properties(
            physical_device, base_info, external_info->handleType,
            &external_props->externalMemoryProperties);
      }
      if (result != VK_SUCCESS)
         goto fail;

      /* pan_image_layout_init requirements for explicit layout */
      base_props->imageFormatProperties.maxMipLevels = 1;
      base_props->imageFormatProperties.maxArrayLayers = 1;
      base_props->imageFormatProperties.sampleCounts = 1;
   }

   if (cubic_props) {
      /* note: blob only allows cubic filtering for 2D and 2D array views
       * its likely we can enable it for 1D and CUBE, needs testing however
       */
      if ((image_view_info->imageViewType == VK_IMAGE_VIEW_TYPE_2D ||
           image_view_info->imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY) &&
          (format_feature_flags &
           VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_CUBIC_BIT_EXT)) {
         cubic_props->filterCubic = true;
         cubic_props->filterCubicMinmax = true;
      } else {
         cubic_props->filterCubic = false;
         cubic_props->filterCubicMinmax = false;
      }
   }

   if (hic_props) {
      VkImageUsageFlags stencil_usage = stencil_usage_info ?
         stencil_usage_info->stencilUsage : base_info->usage;

      /* We don't support AFBC for images used for host transfer. So, if an
       * image could have been tiled as AFBC if it weren't for host transfer,
       * report suboptimal access. */
      VkImageUsageFlags usage = base_info->usage | stencil_usage;
      usage &= ~VK_IMAGE_USAGE_HOST_TRANSFER_BIT;
      bool can_use_afbc = panvk_image_can_use_afbc(
         physical_device, base_info->format, usage, base_info->type,
         base_info->tiling, base_info->flags);
      hic_props->optimalDeviceAccess = !can_use_afbc;

      /* FIXME: we only support host transfer with certain modifiers and for now
       * there's no easy way to know whether the presence of HOST_TRANSFER will
       * be the thing that causes the modifier to be filtered out, and thus
       * causing a difference in memory layout.
       *
       * See https://gitlab.freedesktop.org/panfrost/mesa/-/issues/281 for
       * details.
       */
      hic_props->identicalMemoryLayout = false;
   }

   const struct vk_format_ycbcr_info *ycbcr_info =
      vk_format_get_ycbcr_info(base_info->format);
   const unsigned plane_count =
      vk_format_get_plane_count(base_info->format);

   /* From the Vulkan 1.3.259 spec, VkImageCreateInfo:
    *
    *    VUID-VkImageCreateInfo-imageCreateFormatFeatures-02260
    *
    *    "If format is a multi-planar format, and if imageCreateFormatFeatures
    *    (as defined in Image Creation Limits) does not contain
    *    VK_FORMAT_FEATURE_2_DISJOINT_BIT, then flags must not contain
    *    VK_IMAGE_CREATE_DISJOINT_BIT"
    *
    * This is satisfied trivially because we support DISJOINT on all
    * multi-plane formats.  Also,
    *
    *    VUID-VkImageCreateInfo-format-01577
    *
    *    "If format is not a multi-planar format, and flags does not include
    *    VK_IMAGE_CREATE_ALIAS_BIT, flags must not contain
    *    VK_IMAGE_CREATE_DISJOINT_BIT"
    */
   if (plane_count == 1 &&
       !(base_info->flags & VK_IMAGE_CREATE_ALIAS_BIT) &&
       (base_info->flags & VK_IMAGE_CREATE_DISJOINT_BIT))
      goto fail;

   if (ycbcr_info &&
       ((base_info->flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT) ||
       (base_info->flags & VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT)))
      goto fail;

   if ((base_info->flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT) &&
       (base_info->usage & VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT))
      goto fail;

   if (ycbcr_props)
      ycbcr_props->combinedImageSamplerDescriptorCount = 1;

   return VK_SUCCESS;

fail:
   if (result == VK_ERROR_FORMAT_NOT_SUPPORTED) {
      /* From the Vulkan 1.0.42 spec:
       *
       *    If the combination of parameters to
       *    vkGetPhysicalDeviceImageFormatProperties2 is not supported by
       *    the implementation for use in vkCreateImage, then all members of
       *    imageFormatProperties will be filled with zero.
       */
      base_props->imageFormatProperties = (VkImageFormatProperties){};
   }

   return result;
}

VKAPI_ATTR void VKAPI_CALL
panvk_GetPhysicalDeviceSparseImageFormatProperties2(
   VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo,
   uint32_t *pPropertyCount, VkSparseImageFormatProperties2 *pProperties)
{
   VK_OUTARRAY_MAKE_TYPED(VkSparseImageFormatProperties2, out, pProperties, pPropertyCount);

   VkPhysicalDeviceImageFormatInfo2 img_info = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
      .format = pFormatInfo->format,
      .type = pFormatInfo->type,
      .tiling = pFormatInfo->tiling,
      .usage = pFormatInfo->usage,
      .flags = VK_IMAGE_CREATE_SPARSE_BINDING_BIT |
               VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT,
   };
   VkImageFormatProperties2 img_props = {};
   if (panvk_GetPhysicalDeviceImageFormatProperties2(physicalDevice, &img_info, &img_props) != VK_SUCCESS)
      return;

   if (!(img_props.imageFormatProperties.sampleCounts & pFormatInfo->samples))
      return;

   /*
    * We don't support multisampled sparse partially-resident images for now.
    * Weirdly enough, banning it the obvious way by making
    * get_image_format_properties report sampleCounts of 1 when flags includes
    * SPARSE_RESIDENCY causes "required sample counts not supported" CTS fails,
    * so we ban them here.
    */
   if (pFormatInfo->samples != 1)
      return;

   vk_outarray_append_typed(VkSparseImageFormatProperties2, &out, prop) {
      prop->properties = panvk_get_sparse_image_fmt_props(pFormatInfo->type, pFormatInfo->format);
   }
}

VKAPI_ATTR void VKAPI_CALL
panvk_GetPhysicalDeviceExternalBufferProperties(
   VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo,
   VkExternalBufferProperties *pExternalBufferProperties)
{
   if (pExternalBufferInfo->handleType ==
       VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID) {
      vk_android_get_ahb_buffer_properties(physicalDevice, pExternalBufferInfo,
                                           pExternalBufferProperties);
      return;
   }

   const VkExternalMemoryHandleTypeFlags supported_handle_types =
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT |
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

   /* From the Vulkan 1.3.298 spec:
    *
    *    compatibleHandleTypes must include at least handleType.
    */
   VkExternalMemoryHandleTypeFlags handle_types =
      pExternalBufferInfo->handleType;
   VkExternalMemoryFeatureFlags features = 0;
   if (pExternalBufferInfo->handleType & supported_handle_types) {
      handle_types |= supported_handle_types;
      features |= VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT |
                  VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT;
   }

   pExternalBufferProperties->externalMemoryProperties =
      (VkExternalMemoryProperties){
         .externalMemoryFeatures = features,
         .exportFromImportedHandleTypes = handle_types,
         .compatibleHandleTypes = handle_types,
      };
}
