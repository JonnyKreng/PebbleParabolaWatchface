#include <pebble.h>
#include <time.h>
#include "parabola_draw.h"
#include "message_keys.auto.h"

#define TIME_LAYER_HEIGHT 49
#define DATE_LAYER_HEIGHT 28
#define BATTERY_WIDTH 20
#define BATTERY_HEIGHT 10
#define BATTERY_MARGIN 5
#define WEATHER_LAYER_HEIGHT 20
#define WEATHER_LAYER_WIDTH 80
#define WEATHER_LAYER_MARGIN 5

#define PERSIST_KEY_FG 1
#define PERSIST_KEY_BG 2

#ifdef PBL_ROUND
#define ROUND_MARGIN 45
#endif
#define TIME_DATE_H_MARGIN 10

static Window* s_window;
static TextLayer* s_time_layer;
static TextLayer* s_date_layer;
static TextLayer* s_weather_layer;
static Layer* s_parabola_ul_layer;
static Layer* s_parabola_lr_layer;
static Layer* s_battery_layer;
static Layer* s_separator_layer;

static BatteryChargeState s_battery_charge_state;
static GColor s_fg_color;
static GColor s_bg_color;

static void prv_apply_colors(void)
{
  text_layer_set_text_color(s_time_layer, s_bg_color);
  text_layer_set_text_color(s_date_layer, s_bg_color);
  text_layer_set_text_color(s_weather_layer, s_bg_color);

  parabola_set_foreground_color(s_fg_color);

  layer_mark_dirty(s_parabola_ul_layer);
  layer_mark_dirty(s_parabola_lr_layer);
  layer_mark_dirty(s_battery_layer);
  layer_mark_dirty(s_separator_layer);
}

static void prv_update_time(struct tm* tick_time)
{
  // Update time
  static char s_time_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);

  // Update date
  static char s_date_buffer[16];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a %d %b", tick_time);
  text_layer_set_text(s_date_layer, s_date_buffer);
}

static void prv_separator_layer_update_proc(Layer* layer, GContext* ctx)
{
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_stroke_color(ctx, s_bg_color);
  graphics_draw_line(ctx, GPoint(0, bounds.size.h / 2), GPoint(bounds.size.w, bounds.size.h / 2));
}

static void prv_battery_layer_update_proc(Layer* layer, GContext* ctx)
{
  graphics_context_set_stroke_color(ctx, s_bg_color);
  graphics_context_set_fill_color(ctx, s_bg_color);

  // Battery outline
  GRect bounds = layer_get_bounds(layer);
  graphics_draw_rect(ctx, GRect(0, 0, bounds.size.w - 2, bounds.size.h));
  graphics_draw_rect(ctx, GRect(bounds.size.w - 2, bounds.size.h / 4, 2, bounds.size.h / 2));

  // Battery fill
  int width = (int)((float)s_battery_charge_state.charge_percent / 100.0F * (bounds.size.w - 6));
  graphics_fill_rect(ctx, GRect(2, 2, width, bounds.size.h - 4), 0, GCornerNone);

  // Indicate charging
  if (s_battery_charge_state.is_charging)
  {
    graphics_draw_circle(ctx, GPoint(bounds.size.w / 2, bounds.size.h / 2), bounds.size.h / 2 - 2);
  }
}

static void prv_battery_handler(BatteryChargeState charge_state)
{
  s_battery_charge_state = charge_state;
  layer_mark_dirty(s_battery_layer);
}

static void prv_tick_handler(struct tm* tick_time, TimeUnits units_changed)
{
  prv_update_time(tick_time);
}

static void prv_message_handler(DictionaryIterator* received, void* context)
{
  bool colors_changed = false;

  Tuple* temperature_tuple = dict_find(received, MESSAGE_KEY_weather_temperature);
  if (temperature_tuple)
  {
    text_layer_set_text(s_weather_layer, temperature_tuple->value->cstring);
  }

  Tuple* fg_tuple = dict_find(received, MESSAGE_KEY_foreground_color);
  if (fg_tuple)
  {
    uint32_t color_hex = fg_tuple->value->uint32;
    s_fg_color = GColorFromHEX(color_hex);
    persist_write_int(PERSIST_KEY_FG, color_hex);
    colors_changed = true;
  }

  Tuple* bg_tuple = dict_find(received, MESSAGE_KEY_background_color);
  if (bg_tuple)
  {
    uint32_t color_hex = bg_tuple->value->uint32;
    s_bg_color = GColorFromHEX(color_hex);
    persist_write_int(PERSIST_KEY_BG, color_hex);
    colors_changed = true;
  }

  if (colors_changed)
  {
    prv_apply_colors();
  }
}

static void prv_window_load(Window* window)
{
  Layer* window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Calculate total height for time and date, and their starting Y position to center them
  int total_text_height = TIME_LAYER_HEIGHT + DATE_LAYER_HEIGHT;
  int time_y = (bounds.size.h - total_text_height) / 2;
  int date_y = time_y + TIME_LAYER_HEIGHT;

  // Time Layer
  s_time_layer = text_layer_create(GRect(TIME_DATE_H_MARGIN, time_y, bounds.size.w - TIME_DATE_H_MARGIN * 2,
                                         TIME_LAYER_HEIGHT));
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_time_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Date Layer
  s_date_layer = text_layer_create(GRect(TIME_DATE_H_MARGIN, date_y, bounds.size.w - TIME_DATE_H_MARGIN * 2,
                                         DATE_LAYER_HEIGHT));
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_date_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  // Separator Line under time
  int separator_y = time_y + TIME_LAYER_HEIGHT + 3;
  int separator_width = bounds.size.w / 2;
  s_separator_layer = layer_create(GRect((bounds.size.w - separator_width) / 2, separator_y, separator_width, 1));
  layer_set_update_proc(s_separator_layer, prv_separator_layer_update_proc);
  layer_add_child(window_layer, s_separator_layer);

  // Create and add upper-left parabola layer
  s_parabola_ul_layer = parabola_layer_create_upper_left(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_add_child(window_layer, s_parabola_ul_layer);

  // Create and add lower-right parabola layer
  s_parabola_lr_layer = parabola_layer_create_lower_right(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_add_child(window_layer, s_parabola_lr_layer);

  // Start entrance animations
  parabola_animate_upper_left(s_parabola_ul_layer);
  parabola_animate_lower_right(s_parabola_lr_layer);

  // Battery Layer
#ifdef PBL_ROUND
  GRect battery_bounds = GRect(bounds.size.w - BATTERY_WIDTH - ROUND_MARGIN,
                               BATTERY_MARGIN + ROUND_MARGIN,
                               BATTERY_WIDTH, BATTERY_HEIGHT);
#else
  GRect battery_bounds = GRect(bounds.size.w - BATTERY_WIDTH - BATTERY_MARGIN, BATTERY_MARGIN, BATTERY_WIDTH,
                               BATTERY_HEIGHT);
#endif
  s_battery_layer = layer_create(battery_bounds);
  layer_set_update_proc(s_battery_layer, prv_battery_layer_update_proc);
  layer_add_child(window_layer, s_battery_layer);

  // Weather Layer
#ifdef PBL_ROUND
  GRect weather_bounds = GRect(ROUND_MARGIN,
                               bounds.size.h - WEATHER_LAYER_HEIGHT - WEATHER_LAYER_MARGIN - ROUND_MARGIN,
                               WEATHER_LAYER_WIDTH, WEATHER_LAYER_HEIGHT);
#else
  GRect weather_bounds = GRect(WEATHER_LAYER_MARGIN, bounds.size.h - WEATHER_LAYER_HEIGHT - WEATHER_LAYER_MARGIN,
                               WEATHER_LAYER_WIDTH, WEATHER_LAYER_HEIGHT);
#endif
  s_weather_layer = text_layer_create(weather_bounds);
  text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentLeft);
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text(s_weather_layer, "--°");
  layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));

  // Read persisted colors
s_fg_color = GColorFromHEX(persist_exists(PERSIST_KEY_FG) ? persist_read_int(PERSIST_KEY_FG) : 0xFF0000);
s_bg_color = GColorFromHEX(persist_exists(PERSIST_KEY_BG) ? persist_read_int(PERSIST_KEY_BG) : 0xFFFFFF);

  prv_apply_colors();

  const time_t now = time(NULL);
  struct tm* t = localtime(&now);
  prv_update_time(t);
  prv_battery_handler(battery_state_service_peek());
}

static void prv_window_unload(Window* window)
{
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_weather_layer);
  layer_destroy(s_battery_layer);
  layer_destroy(s_separator_layer);
  parabola_layer_destroy_upper_left(s_parabola_ul_layer);
  parabola_layer_destroy_lower_right(s_parabola_lr_layer);
}

static void prv_init(void)
{
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){
                               .load = prv_window_load,
                               .unload = prv_window_unload,
                             });
  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_handler);
  battery_state_service_subscribe(prv_battery_handler);
  app_message_register_inbox_received(prv_message_handler);
  app_message_open(64, 64);
  window_stack_push(s_window, true);
}

static void prv_deinit(void)
{
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  window_destroy(s_window);
}

int main(void)
{
  prv_init();
  app_event_loop();
  prv_deinit();
}
