#include <pebble.h>
#include "split_flap.h"

static const int FLAP_INSET = 4; // The padding around the flap area itself
static const int FLAP_SPLIT_HEIGHT = 4; // The height of the middle divider
static const int FLAP_ANIMATION_DURATION = 200; // msec
static const GColor FLAP_BACKGROUND_COLOR = GColorWhite; // Colour of the flap background
static const GColor FLAP_FOREGROUND_COLOR = GColorBlack; // Colour of the frame, divider, flap border.

typedef struct SplitFlapLayer {
  // Internal layers
  Layer* layer;
  Layer* mask_layer;
  GBitmap* mask_bmp;

  // Pages
  SplitFlapLayerPage* pages;
  uint32_t num_pages;
  uint32_t current_page;

  SplitFlapLayerCallbacks callbacks;

  // Animation state
  Animation* anim;
  AnimationImplementation* anim_implementation;
  bool anim_forward;
  uint32_t anim_progress;
  uint32_t anim_appearing_page;
  uint32_t anim_disappearing_page;
} SplitFlapLayer;

// Shamelessly copy-pasted from the firmware.
static int32_t animation_timing_function_ease_in_out(uint32_t time_normalized) {
  static const uint16_t ease_in_out_table[32] = {
  0,      136,    545,    1227,
  2182,   3409,   4909,   6682,
  8728,   11047,  13638,  16502,
  19639,  23049,  26731,  30687,
  34847,  38803,  42485,  45895,
  49032,  51896,  54488,  56806,
  58852,  60625,  62126,  63353,
  64308,  64990,  65399,  65535
  };
  const uint8_t index = time_normalized / 2048;
  return ease_in_out_table[index];
}

void split_flap_layer_set_current_page(SplitFlapLayer* split_layer, uint32_t page, bool animated);

static void split_flap_page_layer_setup(SplitFlapLayer* split_layer, SplitFlapLayerPage* page) {
  GRect flapBounds = grect_crop(layer_get_bounds(split_layer->layer), FLAP_INSET);
  int split_y = flapBounds.origin.y + flapBounds.size.h / 2;
  layer_set_frame(page->upperLayer, GRect(flapBounds.origin.x, flapBounds.origin.y, flapBounds.size.w, flapBounds.size.h / 2));
  layer_set_bounds(page->upperLayer, GRect(0, 0, flapBounds.size.w, flapBounds.size.h / 2));
  layer_set_frame(page->lowerLayer, GRect(flapBounds.origin.x, split_y, flapBounds.size.w, flapBounds.size.h / 2));
  layer_set_bounds(page->lowerLayer, GRect(0, 0, flapBounds.size.w, flapBounds.size.h / 2));
}

static void split_flap_animation_cleanup(Animation *animation, bool finished, void *context) {
  SplitFlapLayer* split_layer = (SplitFlapLayer*)context;
  // Finish setting the visibility + call callbacks + update current_page.
  split_flap_layer_set_current_page(split_layer, split_layer->anim_appearing_page, false);
  // Reset the bounds on the old layer.
  split_flap_page_layer_setup(split_layer, &split_layer->pages[split_layer->anim_disappearing_page]);
}

static void split_flap_animation_update(Animation* animation, const uint32_t time_normal) {
  SplitFlapLayer* split_layer = (SplitFlapLayer*)animation_get_context(animation);
  // Make the animation look cool.
  split_layer->anim_progress = animation_timing_function_ease_in_out(time_normal);
  // Invalidate background layer, which is what really implements the animation.
  layer_mark_dirty(split_layer->layer);
}

int32_t split_flap_layer_get_current_page(SplitFlapLayer* split_layer) {
  return split_layer->current_page;
}

void split_flap_layer_set_current_page(SplitFlapLayer* split_layer, uint32_t page_idx, bool animated) {
  if (page_idx == split_layer->current_page) return;
  SplitFlapLayerPage* oldPage = split_layer->pages + split_layer->current_page;
  SplitFlapLayerPage* newPage = split_layer->pages + page_idx;
  if (animated) {
    // The extra convolution is required for proper-feeling wraparound (i.e. the direction it flips always corresponds to the button you press).
    // And, it's about the only phsically accurate part of this entire affair.
    split_layer->anim_forward = (!(page_idx == split_layer->num_pages - 1 && split_layer->current_page == 0) && page_idx > split_layer->current_page) || (page_idx == 0 && split_layer->current_page == split_layer->num_pages - 1);
    split_layer->anim_appearing_page = page_idx;
    split_layer->anim_disappearing_page = split_layer->current_page;

    if (!split_layer->anim) {
      split_layer->anim = animation_create();
      AnimationHandlers callbacks = {
        .started = NULL,
        .stopped = split_flap_animation_cleanup
      };
      animation_set_handlers(split_layer->anim, callbacks, split_layer);

      split_layer->anim_implementation = malloc(sizeof(AnimationImplementation));

      split_layer->anim_implementation->setup = NULL;
      split_layer->anim_implementation->update = split_flap_animation_update;
      split_layer->anim_implementation->teardown = NULL;
      animation_set_implementation(split_layer->anim, split_layer->anim_implementation);

      animation_set_duration(split_layer->anim, FLAP_ANIMATION_DURATION);
    }
    animation_schedule(split_layer->anim);
  } else {
    layer_set_hidden(oldPage->upperLayer, true);
    layer_set_hidden(oldPage->lowerLayer, true);

    layer_set_hidden(newPage->upperLayer, false);
    layer_set_hidden(newPage->lowerLayer, false);

    if (split_layer->callbacks.page_changed) {
      split_layer->callbacks.page_changed(split_layer, split_layer->current_page, page_idx);
    }
    split_layer->current_page = page_idx;
  }
}

void split_flap_layer_set_current_page_by_delta(SplitFlapLayer* split_layer, int32_t delta, bool animated) {
  // Special wrapper function that exists mostly to let people page as fast as they want without breaking the animation too badly.
  int current_idx = split_layer->current_page;
  if (animated && split_layer->anim && animation_is_scheduled(split_layer->anim)) {
    current_idx = split_layer->anim_appearing_page;
    animation_unschedule(split_layer->anim);
  }
  split_flap_layer_set_current_page(split_layer, current_idx + delta >= 0 ? (current_idx + delta) % split_layer->num_pages : -((-delta) % split_layer->num_pages) + split_layer->num_pages + current_idx, animated);
}

static void split_flap_layer_draw_background(Layer *layer, GContext* ctx) {
  GRect bounds = layer_get_bounds(layer);
  GRect flapBounds = grect_crop(bounds, FLAP_INSET);
  SplitFlapLayer* split_layer = *(SplitFlapLayer**)layer_get_data(layer);

  int split_y = flapBounds.origin.y + flapBounds.size.h / 2;
  int split_h = FLAP_SPLIT_HEIGHT;
  // The main background
  graphics_context_set_fill_color(ctx, FLAP_BACKGROUND_COLOR);
  graphics_fill_rect(ctx, flapBounds, 8, GCornersAll);
  // We capture this now to use for masking later on, since you can't create arbitrary bitmaps :(
  if (!split_layer->mask_bmp) {
    split_layer->mask_bmp = malloc(sizeof(GBitmap));
    memcpy(split_layer->mask_bmp, ctx, sizeof(GBitmap));
    int sz = split_layer->mask_bmp->bounds.size.h * split_layer->mask_bmp->row_size_bytes;
    split_layer->mask_bmp->addr = malloc(sz);
    split_layer->mask_bmp->is_heap_allocated = true;
    memcpy(split_layer->mask_bmp->addr, ((GBitmap*)ctx)->addr, sz);
  }

  // The split
  graphics_context_set_fill_color(ctx, FLAP_FOREGROUND_COLOR);
  graphics_fill_rect(ctx, GRect(bounds.origin.x, split_y - split_h / 2, bounds.size.w, split_h), 0, 0);

  // The animation
  if (split_layer->anim && animation_is_scheduled(split_layer->anim)) {
    // All the positioning values for our animation.
    // (under the entirely safe assumption that ANIMATION_NORMALIZED_MIN will forever be 0)
    bool flap_up = (split_layer->anim_progress > ANIMATION_NORMALIZED_MAX / 2) ^ split_layer->anim_forward; // Is the falling/rising flap above or below the half-way split?
    bool finished_half = flap_up ^ split_layer->anim_forward; // Are we more than half-way through the animation?
    int full_flap_h = flapBounds.size.h / 2; // The max height of the flap (occurs at the beginning and end of animation).
    int flap_h = (split_layer->anim_progress - ANIMATION_NORMALIZED_MAX / 2); // How tall the flap is at this point in time.
    flap_h = flap_h < 0 ? -flap_h : flap_h;
    flap_h = (flap_h * flapBounds.size.h) / ANIMATION_NORMALIZED_MAX; // Dammit why no abs().
    int flap_h_inv = full_flap_h - flap_h; // The height of the space "underneath" the flap.

    // Draw the flap border in motion
    GRect flap_rect = GRect(flapBounds.origin.x, flap_up ? split_y - flap_h : split_y + split_h / 2, flapBounds.size.w, flap_up ? flap_h - split_h / 2: flap_h - split_h / 2);
    int flap_corner_rad = (flap_h * 8) / (flapBounds.size.h / 2); // You'd need some seriously good eyesight to notice this changing.

    // You can't set a stroke width, so we have to draw then crop then draw again.
    graphics_fill_rect(ctx, grect_crop(flap_rect, -split_h), flap_corner_rad, flap_up ? GCornersTop : GCornersBottom);
    graphics_context_set_fill_color(ctx, FLAP_BACKGROUND_COLOR);
    graphics_fill_rect(ctx, grect_crop(flap_rect, 0), flap_corner_rad == 0 ? 0 : flap_corner_rad - 1, flap_up ? GCornersTop : GCornersBottom);

    // The pages we'll be working with
    SplitFlapLayerPage* oldPage = split_layer->pages + split_layer->anim_disappearing_page;
    SplitFlapLayerPage* newPage = split_layer->pages + split_layer->anim_appearing_page;

    // First, reduce uneeded drawing (calling these repeadedly doesn't do anything too terrible).
    layer_set_hidden(split_layer->anim_forward ^ !finished_half ? newPage->upperLayer : newPage->lowerLayer, false);
    layer_set_hidden(split_layer->anim_forward ^ !finished_half ? newPage->upperLayer : newPage->lowerLayer, false);
    layer_set_hidden(split_layer->anim_forward ? oldPage->upperLayer : oldPage->lowerLayer, finished_half);
    layer_set_hidden(split_layer->anim_forward ^ !finished_half ? newPage->lowerLayer : newPage->upperLayer, false);

    // Convenience values for when we're clipping layers.
    int flap_pre_half_h_inv = !finished_half ? flap_h_inv : full_flap_h;
    int flap_pre_half_h = !finished_half ? flap_h : 0;
    int flap_post_half_h_inv = !finished_half ? full_flap_h : flap_h_inv;
    int flap_post_half_h = !finished_half ? 0 : flap_h;

    // Then, clip appropriately. Unfortunately, we don't have the ability to directly specify clipping parameters for drawing, so it involves a lot of screwing around with layers.
    // Reveal the first half of the new page
    if (split_layer->anim_forward) {
      layer_set_frame(newPage->upperLayer, GRect(flapBounds.origin.x, flapBounds.origin.y, flapBounds.size.w, flap_pre_half_h_inv));
      layer_set_frame(oldPage->upperLayer, GRect(flapBounds.origin.x, flapBounds.origin.y + flap_pre_half_h_inv, flapBounds.size.w, flap_pre_half_h));
    } else {
      layer_set_frame(oldPage->lowerLayer, GRect(flapBounds.origin.x, split_y, flapBounds.size.w, flap_pre_half_h));
      layer_set_bounds(oldPage->lowerLayer, GRect(0, -flap_pre_half_h_inv, flapBounds.size.w, flapBounds.size.h));
      layer_set_frame(newPage->lowerLayer, GRect(flapBounds.origin.x, split_y + flap_pre_half_h, flapBounds.size.w, flap_pre_half_h_inv));
      layer_set_bounds(newPage->lowerLayer, GRect(0, -flap_pre_half_h, flapBounds.size.w, flapBounds.size.h));
    }
    // Reveal the second half
    if (split_layer->anim_forward) {
      layer_set_frame(newPage->lowerLayer, GRect(flapBounds.origin.x, split_y, flapBounds.size.w, flap_post_half_h));
      layer_set_bounds(newPage->lowerLayer, GRect(0, -flap_post_half_h_inv, flapBounds.size.w, flapBounds.size.h));
      layer_set_frame(oldPage->lowerLayer, GRect(flapBounds.origin.x, split_y + flap_post_half_h, flapBounds.size.w, flap_post_half_h_inv));
      layer_set_bounds(oldPage->lowerLayer, GRect(0, -flap_post_half_h, flapBounds.size.w, flapBounds.size.h));
    } else {
      layer_set_frame(oldPage->upperLayer, GRect(flapBounds.origin.x, flapBounds.origin.y, flapBounds.size.w, flap_post_half_h_inv));
      layer_set_frame(newPage->upperLayer, GRect(flapBounds.origin.x, flapBounds.origin.y + flap_post_half_h_inv, flapBounds.size.w, flap_post_half_h));
    }
  }
}

static void split_flap_layer_draw_mask(Layer *layer, GContext* ctx) {
  GRect bounds = layer_get_bounds(layer);
  SplitFlapLayer* split_layer = *(SplitFlapLayer**)layer_get_data(layer);

  // "I'll just use compositing operations to do this - no worries!" - Me, 20 minutes ago
  // "Hmm, maybe if I can create a seperate buffer to prepare before compositing onto the screen buffer" - Me, 10 minutes ago
  // "Ha ha silly me thinking there'd be a function to create a bitmap" - Me, 5 minutes ago
  if (split_layer->mask_bmp) {
    graphics_context_set_compositing_mode(ctx, GCompOpAnd);
    graphics_draw_bitmap_in_rect(ctx, split_layer->mask_bmp, bounds);
  }
}

void split_flap_layer_prev_page_click_handler(ClickRecognizerRef recognizer, void *context) {
  SplitFlapLayer* split_layer = (SplitFlapLayer*)context;
  split_flap_layer_set_current_page_by_delta(split_layer, -1, true);
}

void split_flap_layer_next_page_click_handler(ClickRecognizerRef recognizer, void *context) {
  SplitFlapLayer* split_layer = (SplitFlapLayer*)context;
  split_flap_layer_set_current_page_by_delta(split_layer, 1, true);
}

static void split_flap_click_config_provider(SplitFlapLayer* split_layer) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 200, split_flap_layer_prev_page_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 200, split_flap_layer_next_page_click_handler);
}

SplitFlapLayer* split_flap_layer_create(GRect frame) {
  SplitFlapLayer* split_layer = malloc(sizeof(SplitFlapLayer));
  memset(split_layer, 0, sizeof(SplitFlapLayer));
  // The Layer struct isn't exposed, so we can't make nice custom controls that can be upcast when appropriate
  // ...and using layer_get_data all the time is stupid.
  split_layer->layer = layer_create_with_data(frame, sizeof(SplitFlapLayer*));
  *(SplitFlapLayer**)layer_get_data(split_layer->layer) = split_layer;

  split_layer->mask_layer = layer_create_with_data(frame, sizeof(SplitFlapLayer*));
  *(SplitFlapLayer**)layer_get_data(split_layer->mask_layer) = split_layer;

  // Setup framing
  layer_set_update_proc(split_layer->layer, split_flap_layer_draw_background);
  layer_set_update_proc(split_layer->mask_layer, split_flap_layer_draw_mask);

  return split_layer;
}

Layer* split_flap_layer_get_layer(SplitFlapLayer* split_layer) {
  return split_layer->layer;
}

void split_flap_set_click_config_onto_window(SplitFlapLayer* split_layer, struct Window *window) {
  window_set_click_config_provider_with_context(window, (ClickConfigProvider) split_flap_click_config_provider, split_layer);
}

void split_flap_layer_init_page(SplitFlapLayer* split_layer, SplitFlapLayerPage* page) {
  // No particular reason for these sizes, split_flap_page_layer_setup overwrites them.
  page->upperLayer = layer_create(GRect(0, 0, 100, 100));
  page->lowerLayer = layer_create(GRect(0, 0, 100, 100));
  split_flap_page_layer_setup(split_layer, page);
}

void split_flap_layer_set_pages(SplitFlapLayer* split_layer, SplitFlapLayerPage* pages, uint32_t num_pages) {
  bool first_use = split_layer->pages != NULL;
  split_layer->pages = pages;
  split_layer->num_pages = num_pages;

  layer_remove_child_layers(split_layer->layer);

  for (uint i = 0; i < num_pages; ++i) {
    layer_set_hidden(pages[i].upperLayer, true);
    layer_set_hidden(pages[i].lowerLayer, true);
    layer_add_child(split_layer->layer, pages[i].upperLayer);
    layer_add_child(split_layer->layer, pages[i].lowerLayer);
    split_flap_page_layer_setup(split_layer, &pages[i]);
  }
  // Adding this as a child again will push it to the top of the z order
  layer_add_child(split_layer->layer, split_layer->mask_layer);

  if (first_use) {
      split_flap_layer_set_current_page(split_layer, 0, false);
  }
}

void split_flap_layer_set_callbacks(SplitFlapLayer* split_layer, SplitFlapLayerCallbacks callbacks) {
  split_layer->callbacks = callbacks;
}

void split_flap_layer_deinit_page(SplitFlapLayerPage* page) {
  layer_destroy(page->upperLayer);
  layer_destroy(page->lowerLayer);
}

void split_flap_layer_destroy(SplitFlapLayer* split_layer) {
  layer_destroy(split_layer->layer);
  layer_destroy(split_layer->mask_layer);
  if (split_layer->anim) {
    free(split_layer->anim_implementation);
    animation_destroy(split_layer->anim);
  }
  if (split_layer->mask_bmp) {
    gbitmap_destroy(split_layer->mask_bmp);
  }
  free(split_layer);
}
