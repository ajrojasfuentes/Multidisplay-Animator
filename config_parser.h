#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "anim_config.h"

// Función para leer y parsear el archivo JSON
AnimationConfig *load_config(const char *filename);
void free_config(AnimationConfig *config);

#endif // CONFIG_PARSER_H