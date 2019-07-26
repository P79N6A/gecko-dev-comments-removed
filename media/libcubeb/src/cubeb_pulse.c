





#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <pulse/pulseaudio.h>
#include "cubeb/cubeb.h"

struct cubeb {
  pa_threaded_mainloop * mainloop;
  pa_context * context;
  int error;
};

struct cubeb_stream {
  struct cubeb * context;
  pa_stream * stream;
  cubeb_data_callback data_callback;
  cubeb_state_callback state_callback;
  void * user_ptr;
  pa_time_event * drain_timer;
  pa_sample_spec sample_spec;
  int shutdown;
};

enum cork_state {
  UNCORK = 0,
  CORK = 1 << 0,
  NOTIFY = 1 << 1
};

static void
context_state_callback(pa_context * c, void * u)
{
  cubeb * ctx = u;
  if (!PA_CONTEXT_IS_GOOD(pa_context_get_state(c))) {
    ctx->error = 1;
  }
  pa_threaded_mainloop_signal(ctx->mainloop, 0);
}

static void
context_notify_callback(pa_context * c, void * u)
{
  cubeb * ctx = u;
  pa_threaded_mainloop_signal(ctx->mainloop, 0);
}

static void
stream_success_callback(pa_stream * s, int success, void * u)
{
  cubeb_stream * stm = u;
  pa_threaded_mainloop_signal(stm->context->mainloop, 0);
}

static void
stream_drain_callback(pa_mainloop_api * a, pa_time_event * e, struct timeval const * tv, void * u)
{
  cubeb_stream * stm = u;
  
  a->time_free(stm->drain_timer);
  stm->drain_timer = NULL;
  stm->state_callback(stm, stm->user_ptr, CUBEB_STATE_DRAINED);
}

static void
stream_state_callback(pa_stream * s, void * u)
{
  cubeb_stream * stm = u;
  if (!PA_STREAM_IS_GOOD(pa_stream_get_state(s))) {
    stm->state_callback(stm, stm->user_ptr, CUBEB_STATE_ERROR);
  }
  pa_threaded_mainloop_signal(stm->context->mainloop, 0);
}

static void
stream_request_callback(pa_stream * s, size_t nbytes, void * u)
{
  cubeb_stream * stm;
  void * buffer;
  size_t size;
  int r;
  long got;
  size_t towrite;
  size_t frame_size;

  stm = u;

  if (stm->shutdown)
    return;

  frame_size = pa_frame_size(&stm->sample_spec);

  assert(nbytes % frame_size == 0);

  towrite = nbytes;

  while (towrite) {
    size = towrite;
    r = pa_stream_begin_write(s, &buffer, &size);
    assert(r == 0);
    assert(size > 0);
    assert(size % frame_size == 0);

    got = stm->data_callback(stm, stm->user_ptr, buffer, size / frame_size);
    if (got < 0) {
      pa_stream_cancel_write(s);
      stm->shutdown = 1;
      return;
    }

    r = pa_stream_write(s, buffer, got * frame_size, NULL, 0, PA_SEEK_RELATIVE);
    assert(r == 0);

    if ((size_t) got < size / frame_size) {
      pa_usec_t latency = 0;
      r = pa_stream_get_latency(s, &latency, NULL);
      if (r == -PA_ERR_NODATA) {
        
        latency = 100 * PA_USEC_PER_MSEC;
      }
      assert(r == 0 || r == -PA_ERR_NODATA);
      
      
      stm->drain_timer = pa_context_rttime_new(stm->context->context, pa_rtclock_now() + 2 * latency, stream_drain_callback, stm);
      stm->shutdown = 1;
      return;
    }

    towrite -= size;
  }

  assert(towrite == 0);
}

static int
wait_until_context_ready(cubeb * ctx)
{
  for (;;) {
    pa_context_state_t state = pa_context_get_state(ctx->context);
    if (!PA_CONTEXT_IS_GOOD(state))
      return -1;
    if (state == PA_CONTEXT_READY)
      break;
    pa_threaded_mainloop_wait(ctx->mainloop);
  }
  return 0;
}

static int
wait_until_stream_ready(cubeb_stream * stm)
{
  for (;;) {
    pa_stream_state_t state = pa_stream_get_state(stm->stream);
    if (!PA_STREAM_IS_GOOD(state))
      return -1;
    if (state == PA_STREAM_READY)
      break;
    pa_threaded_mainloop_wait(stm->context->mainloop);
  }
  return 0;
}

static int
operation_wait(cubeb * ctx, pa_stream * stream, pa_operation * o)
{
  while (pa_operation_get_state(o) == PA_OPERATION_RUNNING) {
    pa_threaded_mainloop_wait(ctx->mainloop);
    if (!PA_CONTEXT_IS_GOOD(pa_context_get_state(ctx->context)))
      return -1;
    if (stream && !PA_STREAM_IS_GOOD(pa_stream_get_state(stream)))
      return -1;
  }
  return 0;
}

static void
stream_cork(cubeb_stream * stm, enum cork_state state)
{
  pa_operation * o;

  pa_threaded_mainloop_lock(stm->context->mainloop);
  o = pa_stream_cork(stm->stream, state & CORK, stream_success_callback, stm);
  if (o) {
    operation_wait(stm->context, stm->stream, o);
    pa_operation_unref(o);
  }
  pa_threaded_mainloop_unlock(stm->context->mainloop);

  if (state & NOTIFY) {
    stm->state_callback(stm, stm->user_ptr,
                        state & CORK ? CUBEB_STATE_STOPPED : CUBEB_STATE_STARTED);
  }
}

int
cubeb_init(cubeb ** context, char const * context_name)
{
  cubeb * ctx;

  *context = NULL;

  ctx = calloc(1, sizeof(*ctx));
  assert(ctx);

  ctx->mainloop = pa_threaded_mainloop_new();
  ctx->context = pa_context_new(pa_threaded_mainloop_get_api(ctx->mainloop), context_name);

  pa_context_set_state_callback(ctx->context, context_state_callback, ctx);
  pa_threaded_mainloop_start(ctx->mainloop);

  pa_threaded_mainloop_lock(ctx->mainloop);
  pa_context_connect(ctx->context, NULL, 0, NULL);

  if (wait_until_context_ready(ctx) != 0) {
    pa_threaded_mainloop_unlock(ctx->mainloop);
    cubeb_destroy(ctx);
    return CUBEB_ERROR;
  }
  pa_threaded_mainloop_unlock(ctx->mainloop);

  *context = ctx;

  return CUBEB_OK;
}

char const *
cubeb_get_backend_id(cubeb * ctx)
{
  return "pulse";
}

void
cubeb_destroy(cubeb * ctx)
{
  pa_operation * o;

  if (ctx->context) {
    pa_threaded_mainloop_lock(ctx->mainloop);
    o = pa_context_drain(ctx->context, context_notify_callback, ctx);
    if (o) {
      operation_wait(ctx, NULL, o);
      pa_operation_unref(o);
    }
    pa_context_set_state_callback(ctx->context, NULL, NULL);
    pa_context_disconnect(ctx->context);
    pa_context_unref(ctx->context);
    pa_threaded_mainloop_unlock(ctx->mainloop);
  }

  if (ctx->mainloop) {
    pa_threaded_mainloop_stop(ctx->mainloop);
    pa_threaded_mainloop_free(ctx->mainloop);
  }

  free(ctx);
}

int
cubeb_stream_init(cubeb * context, cubeb_stream ** stream, char const * stream_name,
                  cubeb_stream_params stream_params, unsigned int latency,
                  cubeb_data_callback data_callback, cubeb_state_callback state_callback,
                  void * user_ptr)
{
  pa_sample_spec ss;
  cubeb_stream * stm;
  pa_operation * o;
  pa_buffer_attr battr;
  pa_channel_map map;
  int r;

  assert(context);

  *stream = NULL;

  if (stream_params.rate < 1 || stream_params.rate > 192000 ||
      stream_params.channels < 1 || stream_params.channels > 32 ||
      latency < 1 || latency > 2000) {
    return CUBEB_ERROR_INVALID_FORMAT;
  }

  switch (stream_params.format) {
  case CUBEB_SAMPLE_S16LE:
    ss.format = PA_SAMPLE_S16LE;
    break;
  case CUBEB_SAMPLE_S16BE:
    ss.format = PA_SAMPLE_S16BE;
    break;
  case CUBEB_SAMPLE_FLOAT32LE:
    ss.format = PA_SAMPLE_FLOAT32LE;
    break;
  case CUBEB_SAMPLE_FLOAT32BE:
    ss.format = PA_SAMPLE_FLOAT32BE;
    break;
  default:
    return CUBEB_ERROR_INVALID_FORMAT;
  }

  ss.rate = stream_params.rate;
  ss.channels = stream_params.channels;

  
  pa_channel_map_init_auto(&map, ss.channels, PA_CHANNEL_MAP_DEFAULT);

  stm = calloc(1, sizeof(*stm));
  assert(stm);

  stm->context = context;

  stm->data_callback = data_callback;
  stm->state_callback = state_callback;
  stm->user_ptr = user_ptr;

  stm->sample_spec = ss;

  battr.maxlength = -1;
  battr.tlength = pa_usec_to_bytes(latency * PA_USEC_PER_MSEC, &stm->sample_spec);
  battr.prebuf = -1;
  battr.minreq = battr.tlength / 4;
  battr.fragsize = -1;

  pa_threaded_mainloop_lock(stm->context->mainloop);
  stm->stream = pa_stream_new(stm->context->context, stream_name, &ss, &map);
  pa_stream_set_state_callback(stm->stream, stream_state_callback, stm);
  pa_stream_set_write_callback(stm->stream, stream_request_callback, stm);
  pa_stream_connect_playback(stm->stream, NULL, &battr,
                             PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_INTERPOLATE_TIMING |
                             PA_STREAM_START_CORKED,
                             NULL, NULL);

  r = wait_until_stream_ready(stm);
  if (r == 0) {
    

    o = pa_stream_update_timing_info(stm->stream, stream_success_callback, stm);
    if (o) {
      r = operation_wait(stm->context, stm->stream, o);
      pa_operation_unref(o);
    }
  }
  pa_threaded_mainloop_unlock(stm->context->mainloop);

  if (r != 0) {
    cubeb_stream_destroy(stm);
    return CUBEB_ERROR;
  }

  *stream = stm;

  return CUBEB_OK;
}

void
cubeb_stream_destroy(cubeb_stream * stm)
{
  if (stm->stream) {
    stream_cork(stm, CORK);

    pa_threaded_mainloop_lock(stm->context->mainloop);

    if (stm->drain_timer) {
      
      pa_threaded_mainloop_get_api(stm->context->mainloop)->time_free(stm->drain_timer);
    }

    pa_stream_set_state_callback(stm->stream, NULL, NULL);
    pa_stream_disconnect(stm->stream);
    pa_stream_unref(stm->stream);
    pa_threaded_mainloop_unlock(stm->context->mainloop);
  }

  free(stm);
}

int
cubeb_stream_start(cubeb_stream * stm)
{
  stream_cork(stm, UNCORK | NOTIFY);
  return CUBEB_OK;
}

int
cubeb_stream_stop(cubeb_stream * stm)
{
  stream_cork(stm, CORK | NOTIFY);
  return CUBEB_OK;
}

int
cubeb_stream_get_position(cubeb_stream * stm, uint64_t * position)
{
  int r;
  pa_usec_t r_usec;
  uint64_t bytes;

  pa_threaded_mainloop_lock(stm->context->mainloop);
  r = pa_stream_get_time(stm->stream, &r_usec);
  pa_threaded_mainloop_unlock(stm->context->mainloop);

  if (r != 0) {
    return CUBEB_ERROR;
  }

  bytes = pa_usec_to_bytes(r_usec, &stm->sample_spec);
  *position = bytes / pa_frame_size(&stm->sample_spec);

  return CUBEB_OK;
}

