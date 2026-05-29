#pragma once

#include <pebble.h>

Layer* parabola_layer_create_upper_left(GRect bounds);
void parabola_layer_destroy_upper_left(Layer* layer);

Layer* parabola_layer_create_lower_right(GRect bounds);
void parabola_layer_destroy_lower_right(Layer* layer);

void parabola_animate_upper_left(Layer* layer);
void parabola_animate_lower_right(Layer* layer);
