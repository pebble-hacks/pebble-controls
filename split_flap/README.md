Split Flap Display Layer for Pebble
===============

There are a few steps to getting split-flap goodness into your app:

1. Create the `SplitFlapLayer`

        SplitFlapLayer* split_flap_layer_create(GRect frame)

1. Allocate some `SplitFlapLayerPage`s and initialize them:

        void split_flap_layer_init_page(SplitFlapLayer* split_layer, SplitFlapLayerPage* page)

1. Add your content layers to each page's `page->upperLayer` and `page->lowerLayer` layers (sorry, you can't use the same layer in both places... yet).
1. Tell the SplitFlapLayer the pages you want it to display:

        void split_flap_layer_set_pages(SplitFlapLayer* split_layer, SplitFlapLayerPage* pages, int32_t num_pages)
    (where `pages` is a pointer to an array of pointers to the `SplitFlapLayerPage`s from step 3)

1. Add the SplitFlapLayer's internal Layer to your UI. For example:

        layer_add_child(window_layer, split_flap_layer_get_layer(my_split_layer))

1. Once you're all done with the layer (during app shutdown, most likely):
    1. Deinit each page before freeing the pointer. For example:

            split_flap_layer_deinit_page(page);
            free(page);

    1. Don't forget to deallocate your page array if it was dynamically allocated.
    1. Finally, call clean up the last internal bits of the control.

            void split_flap_layer_destroy(SplitFlapLayer* split_layer)

Changing Pages
--------------
**`split_flap_layer_set_current_page(SplitFlapLayer* split_layer, int32_t page_idx, bool animated)`**

For when you wish to display a particular page.

**`void split_flap_layer_set_current_page_by_delta(SplitFlapLayer* split_layer, int32_t delta, bool animated)`**

For when you want to change pages relative to the current page.

**`int32_t split_flap_layer_set_current_page(SplitFlapLayer* split_layer)`**

For when you need to know the active page.

Callback
--------
There's exactly one callback, called when the current page changes. If the page change is animated, the callback is called once the animation completes.

Here's how you use it:

    static void flip_page_changed(struct SplitFlapLayer* split_layer, int old_page_idx, int new_page_idx) {
      // The page has changed from old_page_idx to new_page_idx
    }
    ...
    static void setup_split_flap(void) {
      ...
      split_flap_layer_set_callbacks(split_layer, (SplitFlapLayerCallbacks){.page_changed = flip_page_changed});
      ...
    }