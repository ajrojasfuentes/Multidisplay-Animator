#include "anim_utils.h"

Position interpolate_position(Position p0, Position p1, int t, int t_start, int t_end) {
    if (t <= t_start) return p0;
    if (t >= t_end) return p1;

    float alpha = (float)(t - t_start) / (t_end - t_start);
    Position result;
    result.x = p0.x + (int)((p1.x - p0.x) * alpha);
    result.y = p0.y + (int)((p1.y - p0.y) * alpha);
    return result;
}