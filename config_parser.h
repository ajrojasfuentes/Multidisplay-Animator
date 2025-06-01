#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

typedef struct {
    int x, y;
} Position;

typedef struct {
    int t_start, t_end;
    Position pos0, pos1;
    int rows, cols;
    char ***rotations;  // rotaciones[ángulo][fila][columna]
} Figure;

typedef struct {
    int width, height;
} Canvas;

typedef struct {
    int num_figures;
    Canvas canvas;
    Figure *figures;
} AnimationConfig;

// Función para leer y parsear el archivo JSON
AnimationConfig *load_config(const char *filename);
void free_config(AnimationConfig *config);

#endif // CONFIG_PARSER_H