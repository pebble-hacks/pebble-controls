#include <pebble.h>

typedef struct SplitFlapLayer SplitFlapLayer;

typedef void (*SplitFlapLayerPageChangedCallback)(struct SplitFlapLayer* split_layer, int old_page_idx, int new_page_idx);

typedef struct SplitFlapLayerCallbacks {
  SplitFlapLayerPageChangedCallback page_changed;
} SplitFlapLayerCallbacks;

typedef struct SplitFlapLayerPage {
  Layer* upperLayer;
  Layer* lowerLayer;
} SplitFlapLayerPage;

// Create a new split flap layer with no pages.
SplitFlapLayer* split_flap_layer_create(GRect frame);
// Get the underlying Layer*, to add into the main UI.
Layer* split_flap_layer_get_layer(SplitFlapLayer* split_layer);
// Set up the up/down page switch actions (entirely optional).
void split_flap_set_click_config_onto_window(SplitFlapLayer* split_layer, Window* window);
// Initialize a freshly allocated SplitFlapLayerPage.
void split_flap_layer_init_page(SplitFlapLayer* split_layer, SplitFlapLayerPage* page);
// Set the pages visible on the split flap layer.
void split_flap_layer_set_pages(SplitFlapLayer* split_layer, SplitFlapLayerPage* pages, uint32_t num_pages);
// Get the current page index.
int32_t split_flap_layer_get_current_page(SplitFlapLayer* split_layer);
// Set the current page index.
void split_flap_layer_set_current_page(SplitFlapLayer* split_layer, uint32_t page_idx, bool animated);
// Change the current page index based on a delta (|delta| can be larger than num_pages).
void split_flap_layer_set_current_page_by_delta(SplitFlapLayer* split_layer, int32_t delta, bool animated);
// Specify the callbacks (as defined in struct SplitFlapLayerCallbacks).
void split_flap_layer_set_callbacks(SplitFlapLayer* split_layer, SplitFlapLayerCallbacks callbacks);
// Free resources associated with a SplitFlapLayerPage (does not free the page itself).
void split_flap_layer_deinit_page(SplitFlapLayerPage* page);
// Destroys a split flap layer.
void split_flap_layer_destroy(SplitFlapLayer* split_layer);
