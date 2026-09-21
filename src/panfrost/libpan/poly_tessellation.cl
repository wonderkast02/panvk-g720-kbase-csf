/*
 * Copyright 2026 Mesa3D Contributors
 * SPDX-License-Identifier: MIT
 *
 * Precompiled tessellation entrypoints backed by libpoly.
 */

#include "compiler/libcl/libcl_vk.h"
#include "poly/geometry.h"
#include "poly/tessellator.h"
#include "poly/cl/tessellator.h"
#include "poly/cl/restart.h"

/*
 * Prefix-sum the number of generated indices for all patches, allocate the
 * final index buffer from the poly heap, and emit one indexed indirect draw.
 */
/*
 * Prepare the libpoly state for one Vulkan indirect tessellation draw.
 *
 * grids contains PanVK compute workgroup counts:
 *   [0..2] SW-VS, [3..5] TCS.
 * The fixed-function tessellator consumes p->nr_patches separately.
 */
KERNEL(1)
panlib_tess_setup_indirect(
   global struct poly_tess_params *p,
   global struct poly_vertex_params *vp,
   global uint32_t *grids,
   global const uint32_t *indirect,
   global const uint32_t *draw_count_buffer,
   uint32_t draw_index,
   uint64_t in_index_buffer,
   uint32_t in_index_buffer_range_el,
   uint32_t in_index_size_B,
   uint64_t vertex_outputs,
   uint32_t vs_wg_size_x,
   uint32_t vs_wg_size_y,
   global uint32_t *first_vertex_sysval,
   global uint32_t *base_instance_sysval)
{
   const bool active =
      !draw_count_buffer || draw_index < *draw_count_buffer;
   const uint count = active ? indirect[0] : 0;
   const uint instance_count = active ? indirect[1] : 0;
   const uint in_patches =
      p->input_patch_size ? count / p->input_patch_size : 0;
   const uint unrolled_patches = in_patches * instance_count;

   uint alloc = 0;
   const uint tcs_out_offs = alloc;
   alloc += unrolled_patches * p->tcs_stride_el * sizeof(uint32_t);

   const uint coord_offs = alloc;
   alloc += unrolled_patches * sizeof(uint32_t);

   const uint count_offs = alloc;
   alloc += unrolled_patches * sizeof(uint32_t);

   const uint vb_offs = alloc;
   alloc += poly_tcs_in_size(count * instance_count, vertex_outputs);

   global uchar *blob = poly_heap_alloc(p->heap, alloc);

   p->tcs_buffer = (global float *)(blob + tcs_out_offs);
   p->patches_per_instance = in_patches;
   p->coord_allocs = (global uint *)(blob + coord_offs);
   p->nr_patches = unrolled_patches;
   p->counts = (global uint32_t *)(blob + count_offs);

   vp->output_buffer = (uintptr_t)(blob + vb_offs);
   vp->outputs = vertex_outputs;
   poly_vertex_params_set_draw(vp, count, instance_count);
   vp->index_size_B = in_index_size_B;

   if (in_index_size_B) {
      vp->index_buffer =
         poly_index_buffer(in_index_buffer, in_index_buffer_range_el,
                           indirect[2], in_index_size_B);
      vp->index_buffer_range_el =
         poly_index_buffer_range_el(in_index_buffer_range_el, indirect[2]);
   }

   if (first_vertex_sysval)
      *first_vertex_sysval = indirect[in_index_size_B ? 3 : 2];

   if (base_instance_sysval)
      *base_instance_sysval = indirect[in_index_size_B ? 4 : 3];

   grids[0] = vs_wg_size_x
      ? (count / vs_wg_size_x) + ((count % vs_wg_size_x) != 0) : 0;
   grids[1] = vs_wg_size_y
      ? (instance_count / vs_wg_size_y) +
        ((instance_count % vs_wg_size_y) != 0) : 0;
   grids[2] = 1;

   grids[3] = in_patches;
   grids[4] = instance_count;
   grids[5] = 1;
}

/*
 * Prepare one Vulkan indirect VS->GS draw.
 *
 * libpoly uses thread-count/local-size pairs in vp->grid/gp->grid.  PanVK's
 * generic indirect compute dispatcher consumes workgroup counts, so this
 * kernel also emits two 3-word workgroup-count records in grids:
 *   [0..2] SW-VS, [3..5] GS MAIN.
 *
 * The indirect GS path consumes one CPU-selected indirect record for
 * the software VS->GS path. A Vulkan count buffer remains GPU-controlled:
 * record_index >= *draw_count_buffer becomes zero work before any poly-heap
 * allocation. The caller caps emitted children at maxDrawCount.
 */
KERNEL(1)
panlib_gs_setup_indirect(
   constant uint32_t *indirect,
   global const uint32_t *draw_count_buffer,
   uint32_t draw_index,
   global struct poly_vertex_params *vp,
   global struct poly_geometry_params *p,
   global struct poly_heap *heap,
   global uint32_t *grids,
   uint64_t input_index_buffer,
   uint32_t input_index_buffer_range_el,
   uint32_t input_index_size_B,
   uint64_t vs_outputs,
   uint32_t prim,
   uint32_t max_indices,
   uint32_t shape,
   global uint32_t *first_vertex_sysval,
   global uint32_t *base_instance_sysval)
{
   const bool active =
      !draw_count_buffer || draw_index < *draw_count_buffer;

   if (active) {
      poly_gs_setup_indirect(input_index_buffer, indirect, vp, p, heap,
                             vs_outputs, input_index_size_B,
                             input_index_buffer_range_el, prim, 0, max_indices,
                             (enum poly_gs_shape)shape);
   } else {
      /*
       * Keep the CPU-initialized parameter blocks valid but guarantee that
       * every downstream physical stage receives zero work. In particular,
       * do not call poly_gs_setup_indirect(), because that routine can allocate
       * output storage/index streams from the poly heap.
       */
      poly_vertex_params_set_draw(vp, 0, 0);
      poly_geometry_params_set_draw(p, prim, (enum poly_gs_shape)shape,
                                    max_indices, 0, 0);
      vp->index_size_B = input_index_size_B;
      vp->outputs = vs_outputs;
   }

   if (first_vertex_sysval)
      *first_vertex_sysval = active ? indirect[input_index_size_B ? 3 : 2] : 0;

   if (base_instance_sysval)
      *base_instance_sysval = active ? indirect[input_index_size_B ? 4 : 3] : 0;

   for (uint i = 0; i < 3; ++i) {
      const uint vs_local = vp->grid[3 + i];
      const uint gs_local = p->grid[3 + i];
      const uint vs_threads = vp->grid[i];
      const uint gs_threads = p->grid[i];

      grids[i] = vs_local
         ? (vs_threads / vs_local) + ((vs_threads % vs_local) != 0) : 0;
      grids[3 + i] = gs_local
         ? (gs_threads / gs_local) + ((gs_threads % gs_local) != 0) : 0;
   }
}

/*
 * Primitive-restart rewrite for the indirect GS path.
 *
 * Convert one direct or CPU-selected indirect indexed draw with restart into
 * one restart-free indexed indirect list draw backed by the poly heap. For an
 * indirect-count child, perform the runtime-count predicate before entering
 * poly_unroll_restart(), so an inactive record cannot allocate heap storage.
 */
KERNEL(256)
panlib_gs_unroll_restart_basic(
   global struct poly_heap *heap,
   uint64_t index_buffer,
   constant uint32_t *in_draw,
   global const uint32_t *draw_count_buffer,
   uint32_t draw_index,
   global uint32_t *out_draw,
   uint32_t index_buffer_range_el,
   uint32_t index_size_B,
   uint32_t restart_index,
   uint32_t flatshade_first,
   uint32_t prim)
{
   const bool active =
      !draw_count_buffer || draw_index < *draw_count_buffer;

   if (!active) {
      if (cl_local_id.x == 0) {
         for (uint i = 0; i < 5; ++i)
            out_draw[i] = 0;
      }
      return;
   }

   POLY_DECL_UNROLL_RESTART_SCRATCH(scratch, 256);
   poly_unroll_restart(out_draw, heap, in_draw, index_buffer,
                       index_buffer_range_el, index_size_B, restart_index,
                       flatshade_first, (enum mesa_prim)prim, scratch);
}

KERNEL(256)
panlib_prefix_sum_tess(global struct poly_tess_params *p)
{
   local uint scratch[32];

   poly_prefix_sum(scratch, p->counts, p->nr_patches,
                   1 /* words */, 0 /* word */, 256);

   barrier(CLK_LOCAL_MEM_FENCE);

   if (cl_local_id.x != 0)
      return;

   const uint total =
      p->nr_patches > 0 ? p->counts[p->nr_patches - 1] : 0;

   const uint32_t elsize_B = sizeof(uint32_t);
   const uint32_t size_B = total * elsize_B;
   const uint alloc_B = poly_heap_alloc_offs(p->heap, size_B);

   p->index_buffer =
      (global uint32_t *)(((uintptr_t)p->heap->base) + alloc_B);

   global uint32_t *desc = p->out_draws;

   desc[0] = total;              /* indexCount */
   desc[1] = 1;                  /* instanceCount */
   desc[2] = alloc_B / elsize_B; /* firstIndex */
   desc[3] = 0;                  /* vertexOffset */
   desc[4] = 0;                  /* firstInstance */
}

KERNEL(1)
panlib_tess_isoline(constant struct poly_tess_params *p,
                    enum poly_tess_mode mode)
{
   const uint patch = cl_global_id.x;
   poly_tess_isoline_process(p, patch, mode);
}

KERNEL(1)
panlib_tess_tri(constant struct poly_tess_params *p,
                enum poly_tess_mode mode)
{
   const uint patch = cl_global_id.x;
   poly_tess_tri_process(p, patch, mode);
}

KERNEL(1)
panlib_tess_quad(constant struct poly_tess_params *p,
                 enum poly_tess_mode mode)
{
   const uint patch = cl_global_id.x;
   poly_tess_quad_process(p, patch, mode);
}
