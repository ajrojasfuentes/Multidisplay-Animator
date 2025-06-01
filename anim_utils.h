#ifndef ANIM_UTILS_H
#define ANIM_UTILS_H

#include "anim_config.h"

// Interpola la posición entre dos puntos dados un tiempo t
Position interpolate_position(Position p0, Position p1, int t, int t_start, int t_end);

#endif