#ifndef PANVK_BCN_H
#define PANVK_BCN_H
#include <stdbool.h>
#include <stdint.h>
int panvk_bcn_init(void);
bool panvk_bcn_is_bcn_format(uint32_t format);
const unsigned char *panvk_bcn_get_shader(uint32_t format, unsigned int *out_len);
const char *panvk_bcn_get_format_name(uint32_t format);
#endif
