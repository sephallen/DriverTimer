/* 
 * Driver Timer by Joseph Allen.
 * Based on the Pebble Stopwatch code from https://github.com/Katharine/pebble-stopwatch
 */

/*
 * Pebble Stopwatch - the big, ugly file.
 * Copyright (C) 2013 Katharine Berry
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <pebble.h>

// #include "common.h"

static Window* window;

// Main display
static TextLayer* drive_time_label_layer;
static TextLayer* big_time_layer;
static TextLayer* drive_remaining_label_layer;
static TextLayer* remaining_drive_layer;
static TextLayer* rest_time_label_layer;
static TextLayer* big_rest_layer;
static TextLayer* rest_remaining_label_layer;
static TextLayer* remaining_rest_layer;
// static Layer* line_layer;
// static GBitmap* button_bitmap;
// static BitmapLayer* button_labels;
static GFont large_font;
static GFont small_font;
static GFont label_font;
ActionBarLayer *action_bar;
static GBitmap* drive_button;
static GBitmap* rest_button;
static GBitmap* reset_button;

// Actually keeping track of time
static double elapsed_time = 0;
static bool started = false;
static AppTimer* update_timer = NULL;
static double start_time = 0;
static double pause_time = 0;

// Keeping track of rest time
static double rest_elapsed_time = 0;
static bool rest_started = false;
static AppTimer* update_rest_timer = NULL;
static double rest_start_time = 0;
static double pause_rest_time = 0;

// Global animation lock. As long as we only try doing things while
// this is zero, we shouldn't crash the watch.
static int busy_animating = 0;

#define TIMER_UPDATE 1
#define PERSIST_STATE 1
  
#define BUTTON_REST BUTTON_ID_SELECT
#define BUTTON_RUN BUTTON_ID_UP
#define BUTTON_RESET BUTTON_ID_DOWN
  
double float_time_ms() {
	time_t seconds;
	uint16_t milliseconds;
	time_ms(&seconds, &milliseconds);
	return (double)seconds + ((double)milliseconds / 1000.0);
}
	
struct StopwatchState {
	bool started;
	double elapsed_time;
	double start_time;
	double pause_time;
  bool rest_started;
	double rest_elapsed_time;
	double rest_start_time;
	double pause_rest_time;
} __attribute__((__packed__));

void config_provider(Window *window);
void handle_init();
time_t time_seconds();
void stop_stopwatch();
void start_stopwatch();
void toggle_stopwatch_handler(ClickRecognizerRef recognizer, Window *window);
void toggle_rest_stopwatch_handler(ClickRecognizerRef recognizer, Window *window);
void reset_stopwatch_handler(ClickRecognizerRef recognizer, Window *window);
void update_stopwatch();
void update_rest_stopwatch();
void handle_timer(void* data);
void handle_rest_timer(void* data);
int main();
// void draw_line(Layer *me, GContext* ctx);
// void save_lap_time(double seconds, bool animate);
// void lap_time_handler(ClickRecognizerRef recognizer, Window *window);
// void shift_lap_layer(PropertyAnimation** animation, Layer* layer, GRect* target, int distance_multiplier);
// void lap_restored(double time);

void handle_init() {
	window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);
  window_set_fullscreen(window, false);

  // Arrange for user input.
  window_set_click_config_provider(window, (ClickConfigProvider) config_provider);

  // Get our fonts
  large_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_LIGHT_34));
  small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_LIGHT_22));
  label_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_LIGHT_12));

  // Root layer
  Layer *root_layer = window_get_root_layer(window);

  // Set up drive time label
  drive_time_label_layer = text_layer_create(GRect(4, 2, 124, 12));
  text_layer_set_background_color(drive_time_label_layer, GColorClear);
  text_layer_set_font(drive_time_label_layer, label_font);
  text_layer_set_text_color(drive_time_label_layer, GColorWhite);
  text_layer_set_text(drive_time_label_layer, "Drive time");
  text_layer_set_text_alignment(drive_time_label_layer, GTextAlignmentLeft);
  layer_add_child(root_layer, (Layer*)drive_time_label_layer);
  
  // Set up the big timer.
  big_time_layer = text_layer_create(GRect(4, 8, 124, 34));
  text_layer_set_background_color(big_time_layer, GColorClear);
//   text_layer_set_font(big_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_font(big_time_layer, large_font);
  text_layer_set_text_color(big_time_layer, GColorWhite);
  text_layer_set_text(big_time_layer, "0:00:00");
  text_layer_set_text_alignment(big_time_layer, GTextAlignmentLeft);
  layer_add_child(root_layer, (Layer*)big_time_layer);
  
  // Set up drive remaining label
  drive_remaining_label_layer = text_layer_create(GRect(4, 43, 124, 15));
  text_layer_set_background_color(drive_remaining_label_layer, GColorClear);
  text_layer_set_font(drive_remaining_label_layer, label_font);
  text_layer_set_text_color(drive_remaining_label_layer, GColorWhite);
  text_layer_set_text(drive_remaining_label_layer, "Remaining");
  text_layer_set_text_alignment(drive_remaining_label_layer, GTextAlignmentLeft);
  layer_add_child(root_layer, (Layer*)drive_remaining_label_layer);
  
  // Set up remaining drive time layer
  remaining_drive_layer = text_layer_create(GRect(4, 53, 124, 22));
  text_layer_set_background_color(remaining_drive_layer, GColorClear);
//   text_layer_set_font(remaining_drive_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_font(remaining_drive_layer, small_font);
  text_layer_set_text_color(remaining_drive_layer, GColorWhite);
  text_layer_set_text(remaining_drive_layer, "4:30:00");
  text_layer_set_text_alignment(remaining_drive_layer, GTextAlignmentLeft);
  layer_add_child(root_layer, (Layer*)remaining_drive_layer);
  
  // Draw our nice line.
//   line_layer = layer_create(GRect(0, 71, 144, 1));
//   layer_set_update_proc(line_layer, draw_line);
//   layer_add_child(root_layer, line_layer);
  
  // Set up rest time label
  rest_time_label_layer = text_layer_create(GRect(4, 75, 124, 12));
  text_layer_set_background_color(rest_time_label_layer, GColorClear);
  text_layer_set_font(rest_time_label_layer, label_font);
  text_layer_set_text_color(rest_time_label_layer, GColorWhite);
  text_layer_set_text(rest_time_label_layer, "Rest time");
  text_layer_set_text_alignment(rest_time_label_layer, GTextAlignmentLeft);
  layer_add_child(root_layer, (Layer*)rest_time_label_layer);
  
  // Set up the rest timer.
  big_rest_layer = text_layer_create(GRect(4, 81, 124, 34));
  text_layer_set_background_color(big_rest_layer, GColorClear);
//   text_layer_set_font(big_rest_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_font(big_rest_layer, large_font);
  text_layer_set_text_color(big_rest_layer, GColorWhite);
  text_layer_set_text(big_rest_layer, "00:00");
  text_layer_set_text_alignment(big_rest_layer, GTextAlignmentLeft);
  layer_add_child(root_layer, (Layer*)big_rest_layer);
  
  // Set up rest remaining label
  rest_remaining_label_layer = text_layer_create(GRect(4, 115, 124, 15));
  text_layer_set_background_color(rest_remaining_label_layer, GColorClear);
  text_layer_set_font(rest_remaining_label_layer, label_font);
  text_layer_set_text_color(rest_remaining_label_layer, GColorWhite);
  text_layer_set_text(rest_remaining_label_layer, "Remaining");
  text_layer_set_text_alignment(rest_remaining_label_layer, GTextAlignmentLeft);
  layer_add_child(root_layer, (Layer*)rest_remaining_label_layer);
  
  // Set up remaining rest time layer
  remaining_rest_layer = text_layer_create(GRect(4, 125, 124, 22));
  text_layer_set_background_color(remaining_rest_layer, GColorClear);
//   text_layer_set_font(remaining_rest_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_font(remaining_rest_layer, small_font);
  text_layer_set_text_color(remaining_rest_layer, GColorWhite);
  text_layer_set_text(remaining_rest_layer, "45:00");
  text_layer_set_text_alignment(remaining_rest_layer, GTextAlignmentLeft);
  layer_add_child(root_layer, (Layer*)remaining_rest_layer);

  // Set up the lap time layers. These will be made visible later.
//   for(int i = 0; i < LAP_TIME_SIZE; ++i) {
//     lap_layers[i] = text_layer_create(GRect(-139, 52, 139, 30));
//     text_layer_set_background_color(lap_layers[i], GColorClear);
//     //         text_layer_set_font(lap_layers[i], laps_font);
//     text_layer_set_text_color(lap_layers[i], GColorWhite);
//     text_layer_set_text(lap_layers[i], lap_times[i]);
//     layer_add_child(root_layer, (Layer*)lap_layers[i]);
//   }

  // Add some button labels
// 	button_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BUTTON_LABELS);
// 	button_labels = bitmap_layer_create(GRect(130, 10, 14, 136));
// 	bitmap_layer_set_bitmap(button_labels, button_bitmap);
//   layer_add_child(root_layer, (Layer*)button_labels);
  
  // Initialize the action bar:
  action_bar = action_bar_layer_create();
  // Associate the action bar with the window:
  action_bar_layer_add_to_window(action_bar, window);
  // Set the click config provider:
  action_bar_layer_set_click_config_provider(action_bar, (ClickConfigProvider) config_provider);
  // Set action bar background colour
  action_bar_layer_set_background_color(action_bar, GColorWhite);
  // Set button icons
  drive_button = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DRIVE_BUTTON);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, drive_button);
  rest_button = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_REST_BUTTON);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, rest_button);
  reset_button = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RESET_BUTTON);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, reset_button);
  
	struct StopwatchState state;
  if(persist_read_data(PERSIST_STATE, &state, sizeof(state)) != E_DOES_NOT_EXIST) {
		started = state.started;
		start_time = state.start_time;
		elapsed_time = state.elapsed_time;
		pause_time = state.pause_time;
    rest_started = state.rest_started;
		rest_start_time = state.rest_start_time;
		rest_elapsed_time = state.rest_elapsed_time;
		pause_rest_time = state.pause_rest_time;
    update_stopwatch();
		update_rest_stopwatch();
		if(started) {
			update_timer = app_timer_register(100, handle_timer, NULL);
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Started timer to resume persisted state.");
		}
    if(rest_started) {
			update_rest_timer = app_timer_register(100, handle_rest_timer, NULL);
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Started timer to resume persisted state.");
		}
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded persisted state.");
  }
}

void handle_deinit() {
	struct StopwatchState state = (struct StopwatchState){
		.started = started,
		.start_time = start_time,
		.elapsed_time = elapsed_time,
		.pause_time = pause_time,
    .rest_started = rest_started,
		.rest_start_time = rest_start_time,
		.rest_elapsed_time = rest_elapsed_time,
		.pause_rest_time = pause_rest_time,
	};
	status_t status = persist_write_data(PERSIST_STATE, &state, sizeof(state));
	if(status < S_SUCCESS) {
		APP_LOG(APP_LOG_LEVEL_WARNING, "Failed to persist state: %ld", status);
	}
	if(status < S_SUCCESS) {
		APP_LOG(APP_LOG_LEVEL_WARNING, "Failed to persist laps: %ld", status);
	}
	
  action_bar_layer_destroy(action_bar);
  gbitmap_destroy(drive_button);
  gbitmap_destroy(rest_button);
  gbitmap_destroy(reset_button);
// 	bitmap_layer_destroy(button_labels);
// 	gbitmap_destroy(button_bitmap);
// 	layer_destroy(line_layer);
	text_layer_destroy(big_time_layer);
  text_layer_destroy(remaining_drive_layer);
  text_layer_destroy(big_rest_layer);
  text_layer_destroy(remaining_rest_layer);
	fonts_unload_custom_font(large_font);
  fonts_unload_custom_font(small_font);
	window_destroy(window);
}

// void draw_line(Layer *me, GContext* ctx) {
//   graphics_context_set_stroke_color(ctx, GColorWhite);
//   graphics_draw_line(ctx, GPoint(0, 0), GPoint(140, 0));
//   graphics_draw_line(ctx, GPoint(0, 1), GPoint(140, 1));
// }

void stop_stopwatch() {
  started = false;
	pause_time = float_time_ms();
  if(update_timer != NULL) {
    app_timer_cancel(update_timer);
    update_timer = NULL;
  }
}

void stop_rest_stopwatch() {
  rest_started = false;
	pause_rest_time = float_time_ms();
  if(update_rest_timer != NULL) {
    app_timer_cancel(update_rest_timer);
    update_rest_timer = NULL;
  }
}

void start_stopwatch() {
  started = true;
  if(start_time == 0) {
		start_time = float_time_ms();
	} else if(pause_time != 0) {
		double interval = float_time_ms() - pause_time;
		start_time += interval;
	}
  update_timer = app_timer_register(100, handle_timer, NULL);
}

void start_rest_stopwatch() {
  rest_started = true;
  if(rest_start_time == 0) {
		rest_start_time = float_time_ms();
	} else if(pause_rest_time != 0) {
		double rest_interval = float_time_ms() - pause_rest_time;
		rest_start_time += rest_interval;
	}
  update_rest_timer = app_timer_register(100, handle_rest_timer, NULL);
}

void toggle_stopwatch_handler(ClickRecognizerRef recognizer, Window *window) {
  if(started) {
    stop_stopwatch();
  } else {
    start_stopwatch();
  }
  if(rest_started) {
    stop_rest_stopwatch();
  }
  if(rest_elapsed_time < 900) {
    rest_start_time = 0;
    rest_elapsed_time = 0;
    update_rest_stopwatch();
  }
  if(rest_elapsed_time >= 900 ) {
    if(rest_elapsed_time <= 2700) {
      double rest_now = float_time_ms();
      rest_start_time = rest_now - 900;
      rest_elapsed_time = 900;
      update_rest_stopwatch();
    }
  }
  if(rest_elapsed_time >= 2700) {
    if(busy_animating) return;
    bool is_running = started;
    bool rest_is_running = rest_started;
    start_time = 0;
    rest_start_time = 0;
    elapsed_time = 0;
    rest_elapsed_time = 0;
    if(is_running) stop_stopwatch();
    if(rest_is_running) stop_rest_stopwatch();
    update_stopwatch();
    update_rest_stopwatch();
    start_stopwatch();
  }
}

void toggle_rest_stopwatch_handler(ClickRecognizerRef recognizer, Window *window) {
  if(rest_started) {
    stop_rest_stopwatch();
  } else {
    start_rest_stopwatch();
  }
  if(started) {
    stop_stopwatch();
  }
}

void reset_stopwatch_handler(ClickRecognizerRef recognizer, Window *window) {
  if(busy_animating) return;
  bool is_running = started;
  bool rest_is_running = rest_started;
  start_time = 0;
  rest_start_time = 0;
  elapsed_time = 0;
  rest_elapsed_time = 0;
  if(is_running) stop_stopwatch();
  if(rest_is_running) stop_rest_stopwatch();
  update_stopwatch();
  update_rest_stopwatch();
}

    // Animate all the laps away.
//     busy_animating = LAP_TIME_SIZE;
//     static PropertyAnimation* animations[LAP_TIME_SIZE];
//     static GRect targets[LAP_TIME_SIZE];
//     for(int i = 0; i < LAP_TIME_SIZE; ++i) {
//         shift_lap_layer(&animations[i], (Layer*)lap_layers[i], &targets[i], LAP_TIME_SIZE);
//         animation_schedule((Animation*)animations[i]);
//     }
//     next_lap_layer = 0;
// //     clear_stored_laps();
// }

// void lap_time_handler(ClickRecognizerRef recognizer, Window *window) {
//     if(busy_animating) return;
//     double t = elapsed_time - last_lap_time;
//     last_lap_time = elapsed_time;
// 	APP_LOG(APP_LOG_LEVEL_DEBUG, "Elapsed lap time: %d", (int)(t * 100));
//     save_lap_time(t, true);
// }

// Update timer display
void update_stopwatch() {
  
  static char big_time[] = "0:00:00";
  static char remaining_drive[] = "0:00:00";

  // Now convert to hours/minutes/seconds.
  int seconds = (int)elapsed_time % 60;
  int minutes = (int)elapsed_time / 60 % 60;
  int hours = (int)elapsed_time / 3600;
  int rSeconds = (16200 - (int)elapsed_time) % 60;
  int rMinutes = (16200 - (int)elapsed_time) / 60 % 60;
  int rHours = (16200 - (int)elapsed_time) / 3600;
  
  // When one hour of driving time remains, alert user with a short pulse
  if((int)elapsed_time == 12600) {
    vibes_short_pulse();
    return;
  }
  
  // When thirty minutes of driving time remain, alert user with a short pulse
  if((int)elapsed_time == 14400) {
    vibes_short_pulse();
    return;
  }

  // When driver time runs out, stop timer and alert user with a short pulse
  if((int)elapsed_time == 16199) {
    vibes_short_pulse();
    return;
  }
  
  if((int)elapsed_time > 16200) {
    stop_stopwatch();
    vibes_cancel();
    return;
  }

  // Create string from timer and remaining time for display
  snprintf(big_time, 9, "%d:%02d:%02d", hours, minutes, seconds);
  snprintf(remaining_drive, 9, "%d:%02d:%02d", rHours, rMinutes, rSeconds);

  // Now draw the strings.
  text_layer_set_text(big_time_layer, big_time);
  text_layer_set_text(remaining_drive_layer, remaining_drive);
}

// Update rest display
void update_rest_stopwatch() {
  
  static char rest_time[] = "00:00";
  static char remaining_rest[] = "00:00";

  // Now convert to hours/minutes/seconds.
  int rest_seconds = (int)rest_elapsed_time % 60;
  int rest_minutes = (int)rest_elapsed_time / 60 % 60;
  int rest_rSeconds = (2700 - (int)rest_elapsed_time) % 60;
  int rest_rMinutes = (2700 - (int)rest_elapsed_time) / 60 % 60;
  
  // When fifteen minutes of rest time has passed, alert user with a short pulse
  if((int)rest_elapsed_time == 899) {
    vibes_short_pulse();
    return;
  }

  // When rest time is completed, stop timer, alert user with a short pulse and reset drive timer
  if((int)rest_elapsed_time == 2699) {
    vibes_short_pulse();
    return;
  }
  
  if((int)rest_elapsed_time > 2700) {
    stop_rest_stopwatch();
//     vibes_cancel();
    start_time = 0;
    elapsed_time = 0;
    update_stopwatch();
    return;
  }

  // Create string from timer and remaining time for display
  snprintf(rest_time, 9, "%02d:%02d", rest_minutes, rest_seconds);
  snprintf(remaining_rest, 9, "%02d:%02d", rest_rMinutes, rest_rSeconds);

  // Now draw the strings.
  text_layer_set_text(big_rest_layer, rest_time);
  text_layer_set_text(remaining_rest_layer, remaining_rest);
}

void animation_stopped(Animation *animation, void *data) {
	property_animation_destroy((PropertyAnimation*)animation);
    --busy_animating;
}

// void shift_lap_layer(PropertyAnimation** animation, Layer* layer, GRect* target, int distance_multiplier) {
//     GRect origin = layer_get_frame(layer);
//     *target = origin;
//     target->origin.y += target->size.h * distance_multiplier;
// 	if(animation != NULL) {
// 		*animation = property_animation_create_layer_frame(layer, NULL, target);
// 		animation_set_duration((Animation*)*animation, 250);
// 		animation_set_curve((Animation*)*animation, AnimationCurveLinear);
// 		animation_set_handlers((Animation*)*animation, (AnimationHandlers){
// 			.stopped = (AnimationStoppedHandler)animation_stopped
// 		}, NULL);
// 	} else {
// 		layer_set_frame(layer, *target);
// 	}
// }

// void save_lap_time(double lap_time, bool animate) {
//     if(busy_animating && animate) return;

//     static PropertyAnimation* animations[LAP_TIME_SIZE];
//     static GRect targets[LAP_TIME_SIZE];

//     // Shift them down visually (assuming they actually exist)
// 	if(animate) {
// 		busy_animating = LAP_TIME_SIZE;
// 		for(int i = 0; i < LAP_TIME_SIZE; ++i) {
// 			if(i == next_lap_layer) continue; // This is handled separately.
// 			shift_lap_layer(&animations[i], (Layer*)lap_layers[i], &targets[i], 1);
// 			animation_schedule((Animation*)animations[i]);
// 		}
// 	} else {
// 		for(int i = 0; i < LAP_TIME_SIZE; ++i) {
// 			if(i == next_lap_layer) continue; // This is handled separately.
// 			shift_lap_layer(NULL, (Layer*)lap_layers[i], &targets[i], 1);
// 		}
// 	}

//     // Once those are done we can slide our new lap time in.
//     format_lap(lap_time, lap_times[next_lap_layer]);

//     // Animate it
//     static PropertyAnimation* entry_animation;
	
// 	if(animate) {
// 		entry_animation = property_animation_create_layer_frame((Layer*)lap_layers[next_lap_layer], &GRect(-139, 52, 139, 26), &GRect(5, 52, 139, 26));
// 		animation_set_curve((Animation*)entry_animation, AnimationCurveEaseOut);
// 		animation_set_delay((Animation*)entry_animation, 50);
// 		animation_set_handlers((Animation*)entry_animation, (AnimationHandlers){
// 			.stopped = (AnimationStoppedHandler)animation_stopped
// 		}, NULL);
// 		animation_schedule((Animation*)entry_animation);
// 	} else {
// 		layer_set_frame((Layer*)lap_layers[next_lap_layer], GRect(5, 52, 139, 26));
// 	}
//     next_lap_layer = (next_lap_layer + 1) % LAP_TIME_SIZE;

//     // Get it into the laps window, too.
// //     store_lap_time(lap_time);
// }

void handle_timer(void* data) {
	if(started) {
		double now = float_time_ms();
		elapsed_time = now - start_time;
		update_timer = app_timer_register(100, handle_timer, NULL);
	}
	update_stopwatch();
  update_rest_stopwatch();
}

void handle_rest_timer(void* data) {
	if(rest_started) {
		double rest_now = float_time_ms();
		rest_elapsed_time = rest_now - rest_start_time;
		update_rest_timer = app_timer_register(100, handle_rest_timer, NULL);
	}
	update_rest_stopwatch();
  update_stopwatch();
}

void config_provider(Window *window) {
	window_single_click_subscribe(BUTTON_RUN, (ClickHandler)toggle_stopwatch_handler);
	window_single_click_subscribe(BUTTON_RESET, (ClickHandler)reset_stopwatch_handler);
	window_single_click_subscribe(BUTTON_REST, (ClickHandler)toggle_rest_stopwatch_handler);
// 	window_long_click_subscribe(BUTTON_LAP, 700, (ClickHandler)handle_display_lap_times, NULL);
}

int main() {
	handle_init();
	app_event_loop();
	handle_deinit();
	return 0;
}