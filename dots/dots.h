#include <pebble.h>
struct DotsLayer;
typedef struct DotsLayer DotsLayer;

DotsLayer* dots_layer_create(GRect frame);
Layer* dots_layer_get_layer(DotsLayer* dots_layer);
void dots_layer_update(DotsLayer* dots_layer, int8_t num_dots, int8_t active_dot);
void dots_layer_destroy(DotsLayer* dots_layer);
