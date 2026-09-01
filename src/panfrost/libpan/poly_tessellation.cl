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
