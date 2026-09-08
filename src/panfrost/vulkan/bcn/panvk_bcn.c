#include "panvk_bcn.h"
#include "bcn/s3tc_spv.h"
#include "bcn/rgtc_spv.h"
#include "bcn/bc6_spv.h"
#include "bcn/bc7_spv.h"
static const struct {
   uint32_t format; const char *name;
   const unsigned char *spv; unsigned int len;
} bcn_formats[] = {
   {131, "BC1_RGB_UNORM",  bcn_s3tc_spv, bcn_s3tc_spv_len},
   {132, "BC1_RGBA_UNORM", bcn_s3tc_spv, bcn_s3tc_spv_len},
   {133, "BC2_UNORM",      bcn_s3tc_spv, bcn_s3tc_spv_len},
   {134, "BC3_UNORM",      bcn_s3tc_spv, bcn_s3tc_spv_len},
   {135, "BC4_UNORM",      bcn_rgtc_spv, bcn_rgtc_spv_len},
   {136, "BC4_SNORM",      bcn_rgtc_spv, bcn_rgtc_spv_len},
   {137, "BC5_UNORM",      bcn_rgtc_spv, bcn_rgtc_spv_len},
   {138, "BC5_SNORM",      bcn_rgtc_spv, bcn_rgtc_spv_len},
   {140, "BC6H_UFLOAT",    bcn_bc6_spv,  bcn_bc6_spv_len},
   {141, "BC6H_SFLOAT",    bcn_bc6_spv,  bcn_bc6_spv_len},
   {142, "BC7_UNORM",      bcn_bc7_spv,  bcn_bc7_spv_len},
   {143, "BC7_SRGB",       bcn_bc7_spv,  bcn_bc7_spv_len},
   {0, NULL, NULL, 0}
};
int panvk_bcn_init(void) { return 0; }
bool panvk_bcn_is_bcn_format(uint32_t f) {
   for (int i = 0; bcn_formats[i].name; i++) if (bcn_formats[i].format == f) return true;
   return false;
}
const unsigned char *panvk_bcn_get_shader(uint32_t f, unsigned int *out) {
   for (int i = 0; bcn_formats[i].name; i++) if (bcn_formats[i].format == f) {
      if (out) *out = bcn_formats[i].len; return bcn_formats[i].spv;
   }
   if (out) *out = 0; return NULL;
}
const char *panvk_bcn_get_format_name(uint32_t f) {
   for (int i = 0; bcn_formats[i].name; i++) if (bcn_formats[i].format == f) return bcn_formats[i].name;
   return "UNKNOWN";
}
