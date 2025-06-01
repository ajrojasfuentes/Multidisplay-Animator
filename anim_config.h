// anim_config.h
#ifndef ANIM_CONFIG_H
#define ANIM_CONFIG_H

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

#endif // ANIM_CONFIG_H