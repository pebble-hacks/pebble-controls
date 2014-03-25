#include <pebble.h>
#include "dots.h"

typedef struct DotsLayer {
	Layer* layer;
	int8_t num_dots;
	int8_t active_dot;
} DotsLayer;

static const int DOT_RAD = 4;
static const int DOT_INNER_RAD = 2;
static const int DOT_PITCH = 14; // Center-to-center spacing

static void dots_layer_update_proc(Layer *layer, GContext* ctx) {
	GRect bounds = layer_get_bounds(layer);
	DotsLayer* dl = *(DotsLayer**)layer_get_data(layer);
	int dots_left = bounds.size.w / 2 - ((dl->num_dots - 1) * DOT_PITCH) / 2;
	for (int i = 0; i < dl->num_dots; ++i) {
		graphics_context_set_fill_color(ctx, GColorWhite);
		graphics_fill_circle(ctx, GPoint(dots_left + i * DOT_PITCH, DOT_RAD), DOT_RAD);
		if (i == dl->active_dot) {
			graphics_context_set_fill_color(ctx, GColorBlack);
			graphics_fill_circle(ctx, GPoint(dots_left + i * DOT_PITCH, DOT_RAD), DOT_INNER_RAD);
		}
	}
}

DotsLayer* dots_layer_create(GRect frame) {
	DotsLayer* dl = malloc(sizeof(DotsLayer));
	dl->layer = layer_create_with_data(frame, sizeof(DotsLayer*));
	*(DotsLayer**)layer_get_data(dl->layer) = dl;
	layer_set_clips(dl->layer, false);
	dl->num_dots = 0;
	dl->active_dot = 0;

	layer_set_update_proc(dl->layer, dots_layer_update_proc);
	return dl;
}

Layer* dots_layer_get_layer(DotsLayer* dots_layer) {
	return dots_layer->layer;
}

void dots_layer_update(DotsLayer* dots_layer, int8_t num_dots, int8_t active_dot) {
	dots_layer->num_dots = num_dots;
	dots_layer->active_dot = active_dot;
	layer_mark_dirty(dots_layer->layer);
}

void dots_layer_destroy(DotsLayer* dots_layer) {
	layer_destroy(dots_layer->layer);
	free(dots_layer);
}
