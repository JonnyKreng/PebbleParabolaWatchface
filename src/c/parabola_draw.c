#include "parabola_draw.h"

#define NUM_LINES 20
#define PARABOLA_LINE_WIDTH 1
#define ANIMATION_DURATION_MS 2500

typedef struct
{
  Layer* layer;
  int32_t progress;
  Animation* animation;
} ParabolaAnimationState;

static ParabolaAnimationState s_ul_state;
static ParabolaAnimationState s_lr_state;

static void prv_parabola_layer_update_proc_upper_left(Layer* layer, GContext* ctx)
{
  graphics_context_set_stroke_color(ctx, GColorRed);
  graphics_context_set_stroke_width(ctx, PARABOLA_LINE_WIDTH);

  GRect bounds = layer_get_bounds(layer);
  int32_t progress = s_ul_state.layer == layer ? s_ul_state.progress : ANIMATION_NORMALIZED_MAX;

  for (int i = 0; i < NUM_LINES; i++)
  {
    int x1 = i * bounds.size.w / (NUM_LINES - 1) - 1;
    int y1 = 0;
    int x2 = 0;
    int final_y2 = (NUM_LINES - 1 - i) * bounds.size.h / (NUM_LINES - 1) - 1;
    int y2 = final_y2 * progress / ANIMATION_NORMALIZED_MAX;

    graphics_draw_line(ctx, GPoint(x1, y1), GPoint(x2, y2));
  }
}

static void prv_parabola_layer_update_proc_lower_right(Layer* layer, GContext* ctx)
{
  graphics_context_set_stroke_color(ctx, GColorRed);
  graphics_context_set_stroke_width(ctx, PARABOLA_LINE_WIDTH);

  GRect bounds = layer_get_bounds(layer);
  int32_t progress = (s_lr_state.layer == layer) ? s_lr_state.progress : ANIMATION_NORMALIZED_MAX;

  for (int i = 0; i < NUM_LINES; i++)
  {
    int x1 = (bounds.size.w - 1);
    int final_y1 = i * bounds.size.h / (NUM_LINES - 1);
    int x2 = (NUM_LINES - 1 - i) * bounds.size.w / (NUM_LINES - 1);
    int y2 = (bounds.size.h - 1);
    int y1 = (bounds.size.h - 1) + (final_y1 - (bounds.size.h - 1)) * progress / ANIMATION_NORMALIZED_MAX;

    graphics_draw_line(ctx, GPoint(x1, y1), GPoint(x2, y2));
  }
}

static void prv_animation_update(Animation* animation, const AnimationProgress progress)
{
  ParabolaAnimationState* state = animation_get_context(animation);
  if (state && state->layer)
  {
    state->progress = progress;
    layer_mark_dirty(state->layer);
  }
}

static void prv_animation_stopped(Animation* animation, bool finished, void* context)
{
  ParabolaAnimationState* state = context;
  if (state)
  {
    state->animation = NULL;
  }
}

static const AnimationImplementation s_animation_implementation = {
  .update = prv_animation_update,
};

Layer* parabola_layer_create_upper_left(GRect bounds)
{
  s_ul_state.layer = layer_create(bounds);
  s_ul_state.progress = ANIMATION_NORMALIZED_MIN;
  layer_set_update_proc(s_ul_state.layer, prv_parabola_layer_update_proc_upper_left);
  return s_ul_state.layer;
}

void parabola_layer_destroy_upper_left(Layer* layer)
{
  if (s_ul_state.animation)
  {
    animation_unschedule(s_ul_state.animation);
    animation_destroy(s_ul_state.animation);
    s_ul_state.animation = NULL;
  }
  s_ul_state.layer = NULL;
  s_ul_state.progress = ANIMATION_NORMALIZED_MIN;
  layer_destroy(layer);
}

Layer* parabola_layer_create_lower_right(GRect bounds)
{
  s_lr_state.layer = layer_create(bounds);
  s_lr_state.progress = ANIMATION_NORMALIZED_MIN;
  layer_set_update_proc(s_lr_state.layer, prv_parabola_layer_update_proc_lower_right);
  return s_lr_state.layer;
}

void parabola_layer_destroy_lower_right(Layer* layer)
{
  if (s_lr_state.animation)
  {
    animation_unschedule(s_lr_state.animation);
    animation_destroy(s_lr_state.animation);
    s_lr_state.animation = NULL;
  }
  s_lr_state.layer = NULL;
  s_lr_state.progress = ANIMATION_NORMALIZED_MIN;
  layer_destroy(layer);
}

static void parabola_start_animation(ParabolaAnimationState* state)
{
  if (!state->layer || state->animation) return;

  state->progress = ANIMATION_NORMALIZED_MIN;

  Animation* animation = animation_create();
  animation_set_duration(animation, ANIMATION_DURATION_MS);
  animation_set_curve(animation, AnimationCurveLinear);
  animation_set_implementation(animation, &s_animation_implementation);

  AnimationHandlers handlers = {
    .stopped = prv_animation_stopped,
  };
  animation_set_handlers(animation, handlers, state);

  animation_schedule(animation);
  state->animation = animation;
}

void parabola_animate_upper_left(Layer* layer)
{
  parabola_start_animation(&s_ul_state);
}

void parabola_animate_lower_right(Layer* layer)
{
  parabola_start_animation(&s_lr_state);
}
