#include "config_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

static char **parse_rotation_array(cJSON *array, int rows, int cols) {
    if (!cJSON_IsArray(array)) return NULL;

    char **matrix = malloc(sizeof(char *) * rows);
    for (int i = 0; i < rows; ++i) {
        cJSON *row = cJSON_GetArrayItem(array, i);
        if (!cJSON_IsString(row)) {
            matrix[i] = NULL;
            continue;
        }

        matrix[i] = malloc(cols + 1);
        strncpy(matrix[i], row->valuestring, cols);
        matrix[i][cols] = '\0';
    }
    return matrix;
}

AnimationConfig *load_config(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("No se pudo abrir el archivo de configuración");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);

    char *json_str = malloc(fsize + 1);
    fread(json_str, 1, fsize, f);
    json_str[fsize] = 0;
    fclose(f);

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);
    if (!root) {
        fprintf(stderr, "Error parseando JSON\n");
        return NULL;
    }

    AnimationConfig *config = malloc(sizeof(AnimationConfig));
    config->num_figures = cJSON_GetObjectItem(root, "num_figures")->valueint;

    cJSON *canvas = cJSON_GetObjectItem(root, "canvas");
    config->canvas.width = cJSON_GetObjectItem(canvas, "width")->valueint;
    config->canvas.height = cJSON_GetObjectItem(canvas, "height")->valueint;

    cJSON *figures = cJSON_GetObjectItem(root, "figures");
    config->figures = malloc(sizeof(Figure) * config->num_figures);

    for (int i = 0; i < config->num_figures; i++) {
        cJSON *fig = cJSON_GetArrayItem(figures, i);
        Figure *fptr = &config->figures[i];

        fptr->t_start = cJSON_GetObjectItem(fig, "t_start")->valueint;
        fptr->t_end = cJSON_GetObjectItem(fig, "t_end")->valueint;

        cJSON *pos0 = cJSON_GetObjectItem(fig, "pos0");
        cJSON *pos1 = cJSON_GetObjectItem(fig, "pos1");
        fptr->pos0.x = cJSON_GetObjectItem(pos0, "x")->valueint;
        fptr->pos0.y = cJSON_GetObjectItem(pos0, "y")->valueint;
        fptr->pos1.x = cJSON_GetObjectItem(pos1, "x")->valueint;
        fptr->pos1.y = cJSON_GetObjectItem(pos1, "y")->valueint;

        fptr->rows = cJSON_GetObjectItem(fig, "rows")->valueint;
        fptr->cols = cJSON_GetObjectItem(fig, "cols")->valueint;

        fptr->rotations = malloc(sizeof(char **) * 360);
        for (int j = 0; j < 360; j++) fptr->rotations[j] = NULL;

        cJSON *rot = cJSON_GetObjectItem(fig, "rotations");
        fptr->num_rotations = 0;

        int angles[] = {0, 90, 180, 270};
        for (int k = 0; k < 4; k++) {
            char key[4];
            sprintf(key, "%d", angles[k]);
            cJSON *rotation = cJSON_GetObjectItem(rot, key);
            if (rotation) {
                fptr->rotations[angles[k]] = parse_rotation_array(rotation, fptr->rows, fptr->cols);
                fptr->num_rotations++;
            }
        }
    }

    cJSON_Delete(root);
    return config;
}

void free_config(AnimationConfig *config) {
    if (!config) return;
    for (int i = 0; i < config->num_figures; i++) {
        Figure *f = &config->figures[i];
        for (int a = 0; a < 360; a++) {
            if (f->rotations[a]) {
                for (int r = 0; r < f->rows; r++) {
                    free(f->rotations[a][r]);
                }
                free(f->rotations[a]);
            }
        }
        free(f->rotations);
    }
    free(config->figures);
    free(config);
}