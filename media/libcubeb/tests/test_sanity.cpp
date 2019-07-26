





#ifdef NDEBUG
#undef NDEBUG
#endif
#define _XOPEN_SOURCE 500
#include "cubeb/cubeb.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "common.h"

#define STREAM_LATENCY 100
#define STREAM_RATE 44100
#define STREAM_CHANNELS 1
#define STREAM_FORMAT CUBEB_SAMPLE_S16LE

static int dummy;
static uint64_t total_frames_written;
static int delay_callback;

static long
test_data_callback(cubeb_stream * stm, void * user_ptr, void * p, long nframes)
{
  assert(stm && user_ptr == &dummy && p && nframes > 0);
  memset(p, 0, nframes * sizeof(short));
  total_frames_written += nframes;
  if (delay_callback) {
    delay(10);
  }
  return nframes;
}

void
test_state_callback(cubeb_stream * stm, void * user_ptr, cubeb_state state)
{
}

static void
test_init_destroy_context(void)
{
  int r;
  cubeb * ctx;

  r = cubeb_init(&ctx, "test_sanity");
  assert(r == 0 && ctx);

  cubeb_destroy(ctx);
}

static void
test_init_destroy_multiple_contexts(void)
{
  int i;
  int r;
  cubeb * ctx[4];

  for (i = 0; i < 4; ++i) {
    r = cubeb_init(&ctx[i], NULL);
    assert(r == 0 && ctx[i]);
  }

  
  cubeb_destroy(ctx[2]);
  cubeb_destroy(ctx[0]);
  cubeb_destroy(ctx[3]);
  cubeb_destroy(ctx[1]);
}

static void
test_init_destroy_stream(void)
{
  int r;
  cubeb * ctx;
  cubeb_stream * stream;
  cubeb_stream_params params;

  r = cubeb_init(&ctx, "test_sanity");
  assert(r == 0 && ctx);

  params.format = STREAM_FORMAT;
  params.rate = STREAM_RATE;
  params.channels = STREAM_CHANNELS;

  r = cubeb_stream_init(ctx, &stream, "test", params, STREAM_LATENCY,
                        test_data_callback, test_state_callback, &dummy);
  assert(r == 0 && stream);

  cubeb_stream_destroy(stream);
  cubeb_destroy(ctx);
}

static void
test_init_destroy_multiple_streams(void)
{
  int i;
  int r;
  cubeb * ctx;
  cubeb_stream * stream[16];
  cubeb_stream_params params;

  r = cubeb_init(&ctx, "test_sanity");
  assert(r == 0 && ctx);

  params.format = STREAM_FORMAT;
  params.rate = STREAM_RATE;
  params.channels = STREAM_CHANNELS;

  for (i = 0; i < 16; ++i) {
    r = cubeb_stream_init(ctx, &stream[i], "test", params, STREAM_LATENCY,
                          test_data_callback, test_state_callback, &dummy);
    assert(r == 0 && stream[i]);
  }

  for (i = 0; i < 16; ++i) {
    cubeb_stream_destroy(stream[i]);
  }

  cubeb_destroy(ctx);
}

static void
test_init_start_stop_destroy_multiple_streams(int early, int delay_ms)
{
  int i;
  int r;
  cubeb * ctx;
  cubeb_stream * stream[16];
  cubeb_stream_params params;

  r = cubeb_init(&ctx, "test_sanity");
  assert(r == 0 && ctx);

  params.format = STREAM_FORMAT;
  params.rate = STREAM_RATE;
  params.channels = STREAM_CHANNELS;

  for (i = 0; i < 16; ++i) {
    r = cubeb_stream_init(ctx, &stream[i], "test", params, STREAM_LATENCY,
                          test_data_callback, test_state_callback, &dummy);
    assert(r == 0 && stream[i]);
    if (early) {
      r = cubeb_stream_start(stream[i]);
      assert(r == 0);
    }
  }


  if (!early) {
    for (i = 0; i < 16; ++i) {
      r = cubeb_stream_start(stream[i]);
      assert(r == 0);
    }
  }

  if (delay_ms) {
    delay(delay_ms);
  }

  if (!early) {
    for (i = 0; i < 16; ++i) {
      r = cubeb_stream_stop(stream[i]);
      assert(r == 0);
    }
  }

  for (i = 0; i < 16; ++i) {
    if (early) {
      r = cubeb_stream_stop(stream[i]);
      assert(r == 0);
    }
    cubeb_stream_destroy(stream[i]);
  }

  cubeb_destroy(ctx);
}

static void
test_init_destroy_multiple_contexts_and_streams(void)
{
  int i, j;
  int r;
  cubeb * ctx[4];
  cubeb_stream * stream[16];
  cubeb_stream_params params;

  params.format = STREAM_FORMAT;
  params.rate = STREAM_RATE;
  params.channels = STREAM_CHANNELS;

  for (i = 0; i < 4; ++i) {
    r = cubeb_init(&ctx[i], "test_sanity");
    assert(r == 0 && ctx[i]);

    for (j = 0; j < 4; ++j) {
      r = cubeb_stream_init(ctx[i], &stream[i * 4 + j], "test", params, STREAM_LATENCY,
                            test_data_callback, test_state_callback, &dummy);
      assert(r == 0 && stream[i * 4 + j]);
    }
  }

  for (i = 0; i < 4; ++i) {
    for (j = 0; j < 4; ++j) {
      cubeb_stream_destroy(stream[i * 4 + j]);
    }
    cubeb_destroy(ctx[i]);
  }
}

static void
test_basic_stream_operations(void)
{
  int r;
  cubeb * ctx;
  cubeb_stream * stream;
  cubeb_stream_params params;
  uint64_t position;

  r = cubeb_init(&ctx, "test_sanity");
  assert(r == 0 && ctx);

  params.format = STREAM_FORMAT;
  params.rate = STREAM_RATE;
  params.channels = STREAM_CHANNELS;

  r = cubeb_stream_init(ctx, &stream, "test", params, STREAM_LATENCY,
                        test_data_callback, test_state_callback, &dummy);
  assert(r == 0 && stream);

  
  r = cubeb_stream_get_position(stream, &position);
  assert(r == 0 && position == 0);

  r = cubeb_stream_start(stream);
  assert(r == 0);

  
  r = cubeb_stream_get_position(stream, &position);
  assert(r == 0);

  r = cubeb_stream_stop(stream);
  assert(r == 0);

  
  r = cubeb_stream_get_position(stream, &position);
  assert(r == 0);

  cubeb_stream_destroy(stream);
  cubeb_destroy(ctx);
}

static void
test_stream_position(void)
{
  int i;
  int r;
  cubeb * ctx;
  cubeb_stream * stream;
  cubeb_stream_params params;
  uint64_t position, last_position;

  total_frames_written = 0;

  r = cubeb_init(&ctx, "test_sanity");
  assert(r == 0 && ctx);

  params.format = STREAM_FORMAT;
  params.rate = STREAM_RATE;
  params.channels = STREAM_CHANNELS;

  r = cubeb_stream_init(ctx, &stream, "test", params, STREAM_LATENCY,
                        test_data_callback, test_state_callback, &dummy);
  assert(r == 0 && stream);

  
  r = cubeb_stream_get_position(stream, &position);
  assert(r == 0 && position == 0);

  delay(500);

  r = cubeb_stream_get_position(stream, &position);
  assert(r == 0 && position == 0);

  
  r = cubeb_stream_start(stream);
  assert(r == 0);

  
  delay(500);

  
  assert(total_frames_written > 0);

  r = cubeb_stream_get_position(stream, &position);
  assert(r == 0);
  last_position = position;

  delay(500);

  r = cubeb_stream_get_position(stream, &position);
  assert(r == 0);
  assert(position >= last_position);
  last_position = position;

  
  for (i = 0; i < 5; ++i) {
    r = cubeb_stream_get_position(stream, &position);
    assert(r == 0);
    assert(position >= last_position);
    assert(position <= total_frames_written);
    last_position = position;
    delay(500);
  }

  assert(last_position != 0);

  
  r = cubeb_stream_stop(stream);
  assert(r == 0);

  
  delay(500);

  r = cubeb_stream_get_position(stream, &position);
  assert(r == 0);
  last_position = position;

  delay(500);

  r = cubeb_stream_get_position(stream, &position);
  assert(r == 0);
  assert(position == last_position);

  cubeb_stream_destroy(stream);
  cubeb_destroy(ctx);
}

static int do_drain;
static int got_drain;

static long
test_drain_data_callback(cubeb_stream * stm, void * user_ptr, void * p, long nframes)
{
  assert(stm && user_ptr == &dummy && p && nframes > 0);
  if (do_drain == 1) {
    do_drain = 2;
    return 0;
  }
  
  assert(do_drain != 2);
  memset(p, 0, nframes * sizeof(short));
  total_frames_written += nframes;
  return nframes;
}

void
test_drain_state_callback(cubeb_stream * stm, void * user_ptr, cubeb_state state)
{
  if (state == CUBEB_STATE_DRAINED) {
    assert(!got_drain);
    got_drain = 1;
  }
}

static void
test_drain(void)
{
  int r;
  cubeb * ctx;
  cubeb_stream * stream;
  cubeb_stream_params params;
  uint64_t position;

  total_frames_written = 0;

  r = cubeb_init(&ctx, "test_sanity");
  assert(r == 0 && ctx);

  params.format = STREAM_FORMAT;
  params.rate = STREAM_RATE;
  params.channels = STREAM_CHANNELS;

  r = cubeb_stream_init(ctx, &stream, "test", params, STREAM_LATENCY,
                        test_drain_data_callback, test_drain_state_callback, &dummy);
  assert(r == 0 && stream);

  r = cubeb_stream_start(stream);
  assert(r == 0);

  delay(500);

  do_drain = 1;

  for (;;) {
    r = cubeb_stream_get_position(stream, &position);
    assert(r == 0);
    if (got_drain) {
      break;
    } else {
      
      
      const char * backend_id = cubeb_get_backend_id(ctx);
      if (strcmp(backend_id, "audiounit") != 0 &&
          strcmp(backend_id, "winmm") != 0) {
        

        int latency = (STREAM_LATENCY * STREAM_RATE) / 1000;
        assert(position + latency <= total_frames_written);
      }
    }
    delay(500);
  }

  r = cubeb_stream_get_position(stream, &position);
  assert(r == 0);
  assert(got_drain);

  
  

  cubeb_stream_destroy(stream);
  cubeb_destroy(ctx);
}

static void
progress(void)
{
  printf(".");
  fflush(stdout);
}

int
main(int argc, char * argv[])
{
  test_init_destroy_context(); progress();
  test_init_destroy_multiple_contexts(); progress();
  test_init_destroy_stream(); progress();
  test_init_destroy_multiple_streams(); progress();
  test_init_destroy_multiple_contexts_and_streams(); progress();
  test_basic_stream_operations(); progress();
  test_stream_position(); progress();
  delay_callback = 0;
  test_init_start_stop_destroy_multiple_streams(0, 0); progress();
  test_init_start_stop_destroy_multiple_streams(1, 0); progress();
  test_init_start_stop_destroy_multiple_streams(0, 150); progress();
  test_init_start_stop_destroy_multiple_streams(1, 150); progress();
  delay_callback = 1;
  test_init_start_stop_destroy_multiple_streams(0, 0); progress();
  test_init_start_stop_destroy_multiple_streams(1, 0); progress();
  test_init_start_stop_destroy_multiple_streams(0, 150); progress();
  test_init_start_stop_destroy_multiple_streams(1, 150); progress();
  delay_callback = 0;
  test_drain();





  printf("\n");
  return 0;
}
