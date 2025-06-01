// config_parser.c
#include "config_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

// Función auxiliar para parsear una rotación
static char **parse_rotation_array(cJSON *array, int rows, int cols) {
    if (!cJSON_IsArray(array)) return NULL;

    char **matrix = malloc(sizeof(char *) * rows);
    for (int i = 0; i < rows; ++i) {
        cJSON *row = cJSON_GetArrayItem(array, i);
        if (!cJSON_IsString(row)) {
            matrix[i] = NULL;
            continue;
        }

        const char *line = row->valuestring;
        matrix[i] = malloc(sizeof(char) * (cols + 1));
        strncpy(matrix[i], line, cols);
        matrix[i][cols] = '\0';  // Asegura terminación nula
    }
    return matrix;
}

AnimationConfig *load_config(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    char *data = malloc(length + 1);
    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);

    cJSON *root = cJSON_Parse(data);
    free(data);
    if (!root) return NULL;

    AnimationConfig *config = malloc(sizeof(AnimationConfig));
    config->num_figures = cJSON_GetObjectItem(root, "num_figures")->valueint;

    cJSON *canvas = cJSON_GetObjectItem(root, "canvas");
    config->canvas.width = cJSON_GetObjectItem(canvas, "width")->valueint;
    config->canvas.height = cJSON_GetObjectItem(canvas, "height")->valueint;

    config->figures = malloc(sizeof(Figure) * config->num_figures);
    cJSON *figures = cJSON_GetObjectItem(root, "figures");

    for (int i = 0; i < config->num_figures; ++i) {
        cJSON *fig = cJSON_GetArrayItem(figures, i);
        Figure *f = &config->figures[i];

        f->t_start = cJSON_GetObjectItem(fig, "t_start")->valueint;
        f->t_end = cJSON_GetObjectItem(fig, "t_end")->valueint;

        cJSON *pos0 = cJSON_GetObjectItem(fig, "pos0");
        f->pos0.x = cJSON_GetObjectItem(pos0, "x")->valueint;
        f->pos0.y = cJSON_GetObjectItem(pos0, "y")->valueint;

        cJSON *pos1 = cJSON_GetObjectItem(fig, "pos1");
        f->pos1.x = cJSON_GetObjectItem(pos1, "x")->valueint;
        f->pos1.y = cJSON_GetObjectItem(pos1, "y")->valueint;

        f->rows = cJSON_GetObjectItem(fig, "rows")->valueint;
        f->cols = cJSON_GetObjectItem(fig, "cols")->valueint;

        cJSON *rot = cJSON_GetObjectItem(fig, "rotations");
        f->rotations = malloc(sizeof(char **) * 360);
        for (int j = 0; j < 360; j++) f->rotations[j] = NULL;

        // Cargar rotaciones disponibles
        cJSON *rotation0 = cJSON_GetObjectItem(rot, "0");
        if (rotation0) f->rotations[0] = parse_rotation_array(rotation0, f->rows, f->cols);

        cJSON *rotation90 = cJSON_GetObjectItem(rot, "90");
        if (rotation90) f->rotations[90] = parse_rotation_array(rotation90, f->rows, f->cols);

        cJSON *rotation180 = cJSON_GetObjectItem(rot, "180");
        if (rotation180) f->rotations[180] = parse_rotation_array(rotation180, f->rows, f->cols);

        cJSON *rotation270 = cJSON_GetObjectItem(rot, "270");
        if (rotation270) f->rotations[270] = parse_rotation_array(rotation270, f->rows, f->cols);
    }

    cJSON_Delete(root);
    return config;
}

void free_config(AnimationConfig *config) {
    if (!config) return;

    for (int i = 0; i < config->num_figures; ++i) {
        Figure *f = &config->figures[i];
        for (int angle = 0; angle < 360; ++angle) {
            if (f->rotations[angle]) {
                for (int r = 0; r < f->rows; ++r) {
                    free(f->rotations[angle][r]);
                }
                free(f->rotations[angle]);
            }
        }
        free(f->rotations);
    }

    free(config->figures);
    free(config);
}