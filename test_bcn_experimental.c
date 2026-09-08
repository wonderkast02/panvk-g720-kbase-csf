/*
 * test_bcn_experimental.c - Test experimental de la capa BCn
 *
 * Basada en los SPIR-V de Leegao (https://github.com/leegao)
 * Créditos a Leegao por los shaders originales.
 *
 * ESTA ES UNA CAPA EXPERIMENTAL - Requiere pruebas extensas.
 * No usar en producción hasta validar con CTS.
 */

#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>

/* Formatos BCn soportados */
#define BCN_FORMAT_BC1  131
#define BCN_FORMAT_BC3  135
#define BCN_FORMAT_BC4  140
#define BCN_FORMAT_BC6H 142

/* Prototipos de la capa BCn */
extern int panvk_bcn_is_bcn_format(uint32_t format);
extern const char *panvk_bcn_get_format_name(uint32_t format);
extern const void *panvk_bcn_get_shader(uint32_t format, uint32_t *size);

int main(void)
{
   printf("=== TEST BCn EXPERIMENTAL ===\n");
   printf("Basado en SPIR-V de Leegao\n");
   printf("Capa EXPERIMENTAL - Requiere pruebas\n\n");

   void *handle = dlopen("libvulkan_panfrost_kbase.so", RTLD_NOW | RTLD_GLOBAL);
   if (!handle) {
      printf("FAIL: dlopen: %s\n", dlerror());
      return 1;
   }

   int (*is_bcn)(uint32_t) = dlsym(handle, "panvk_bcn_is_bcn_format");
   const char *(*get_name)(uint32_t) = dlsym(handle, "panvk_bcn_get_format_name");
   const void *(*get_shader)(uint32_t, uint32_t*) = dlsym(handle, "panvk_bcn_get_shader");

   if (!is_bcn || !get_name || !get_shader) {
      printf("FAIL: No se encontraron las funciones BCn\n");
      return 1;
   }

   uint32_t formats[] = {BCN_FORMAT_BC1, BCN_FORMAT_BC3, BCN_FORMAT_BC4, BCN_FORMAT_BC6H, 100};
   const char *names[] = {"BC1", "BC3", "BC4", "BC6H", "NO_BCN"};

   for (int i = 0; i < 5; i++) {
      uint32_t size = 0;
      const void *shader = get_shader(formats[i], &size);
      printf("Format %d (%s): BCn=%s, Shader=%s (%u bytes)\n",
             formats[i], names[i],
             is_bcn(formats[i]) ? "YES" : "NO",
             shader ? "FOUND" : "NOT", size);
   }

   printf("\n=== TEST BCn COMPLETADO ===\n");
   printf("Capa EXPERIMENTAL - Basada en SPIR-V de Leegao\n");
   printf("Créditos: Leegao (https://github.com/leegao)\n");

   dlclose(handle);
   return 0;
}
