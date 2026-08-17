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
KERNEL(1024)
panlib_prefix_sum_tess(global struct poly_tess_params *p)
{
   local uint scratch[32];

   poly_prefix_sum(scratch, p->counts, p->nr_patches,
                   1 /* words */, 0 /* word */, 1024);

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
