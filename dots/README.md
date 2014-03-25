Dots Page Indicator Layer for Pebble
===============

Just like the ones used by the notification popup.

1. Create the `DotsLayer`

        DotsLayer* dots_layer_create(GRect frame);

1. Configure the number of dots and the current active dot (you can do this whenever you want while the `DotsLayer` exists).

        dots_layer_update(my_dots_layer, number_of_dots, active_dot_index);

1. Add the `DotsLayer` to your UI. For example:

        layer_add_child(window_layer, dots_layer_get_layer(my_dots_layer));

1. Once you're all done with the layer (during app shutdown, most likely), clean up the dots layer:

        dots_layer_destroy(my_dots_layer);

Configuration
-------------
Check out the top of `dots.c` to configure the dot radius, inner radius, and spacing.