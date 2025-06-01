#ifndef ANIMATOR_MT_H
#define ANIMATOR_MT_H

#include "anim_config.h"

// Animación distribuida usando hilos con mypthreads
void simulate_animation_multithread(const AnimationConfig *config);

#endif // ANIMATOR_MT_H