






#define NDEBUG
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <dlfcn.h>
#include "android/log.h"

#include "cubeb/cubeb.h"
#include "cubeb-internal.h"
#include "android/audiotrack_definitions.h"

#ifndef ALOG
#if defined(DEBUG) || defined(FORCE_ALOG)
#define ALOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gecko - Cubeb" , ## args)
#else
#define ALOG(args...)
#endif
#endif



#define SIZE_AUDIOTRACK_INSTANCE 256





#define DLSYM_DLERROR(mangled_name, pointer, lib)                        \
  do {                                                                   \
    pointer = dlsym(lib, mangled_name);                                  \
    if (!pointer) {                                                      \
      ALOG("error while loading %stm: %stm\n", mangled_name, dlerror()); \
    } else {                                                             \
      ALOG("%stm: OK", mangled_name);                                    \
    }                                                                    \
  } while(0);

static struct cubeb_ops const audiotrack_ops;
void audiotrack_destroy(cubeb * context);
void audiotrack_stream_destroy(cubeb_stream * stream);

struct AudioTrack {
               
   status_t (*get_min_frame_count)(int* frame_count, int stream_type, uint32_t rate);
               

               void* (*ctor)(void* instance, int, unsigned int, int, int, int, unsigned int, void (*)(int, void*, void*), void*, int, int);
               void* (*ctor_froyo)(void* instance, int, unsigned int, int, int, int, unsigned int, void (*)(int, void*, void*), void*, int);
               void* (*dtor)(void* instance);
               void (*start)(void* instance);
               void (*pause)(void* instance);
               uint32_t (*latency)(void* instance);
               status_t (*check)(void* instance);
               status_t (*get_position)(void* instance, uint32_t* position);
              
   int (*get_output_frame_count)(int* frame_count, int stream);
   int (*get_output_latency)(uint32_t* frame_count, int stream);
   int (*get_output_samplingrate)(int* frame_count, int stream);
               status_t (*set_marker_position)(void* instance, unsigned int);

};

struct cubeb {
  struct cubeb_ops const * ops;
  void * library;
  struct AudioTrack klass;
};

struct cubeb_stream {
  cubeb * context;
  cubeb_stream_params params;
  cubeb_data_callback data_callback;
  cubeb_state_callback state_callback;
  void * instance;
  void * user_ptr;
  
  long unsigned written;
  int draining;
};

static void
audiotrack_refill(int event, void* user, void* info)
{
  cubeb_stream * stream = user;
  switch (event) {
    case EVENT_MORE_DATA: {
      long got = 0;
      struct Buffer * b = (struct Buffer*)info;

      if (stream->draining) {
        return;
      }

      got = stream->data_callback(stream, stream->user_ptr, b->raw, b->frameCount);

      stream->written += got;

      if (got != (long)b->frameCount) {
        uint32_t p;
        stream->draining = 1;
        

        stream->context->klass.set_marker_position(stream->instance, stream->written);
      }

      break;
    }
    case EVENT_UNDERRUN:
      ALOG("underrun in cubeb backend.");
      break;
    case EVENT_LOOP_END:
      assert(0 && "We don't support the loop feature of audiotrack.");
      break;
    case EVENT_MARKER:
      assert(stream->draining);
      stream->state_callback(stream, stream->user_ptr, CUBEB_STATE_DRAINED);
      break;
    case EVENT_NEW_POS:
      assert(0 && "We don't support the setPositionUpdatePeriod feature of audiotrack.");
      break;
    case EVENT_BUFFER_END:
      assert(0 && "Should not happen.");
      break;
  }
}


static int
audiotrack_version_is_froyo(cubeb * ctx)
{
  return ctx->klass.ctor_froyo != NULL;
}

int
audiotrack_get_min_frame_count(cubeb * ctx, cubeb_stream_params * params, int * min_frame_count)
{
  status_t status;
  
  if (audiotrack_version_is_froyo(ctx)) {
    int samplerate, frame_count, latency, min_buffer_count;
    status = ctx->klass.get_output_frame_count(&frame_count, AUDIO_STREAM_TYPE_MUSIC);
    if (status) {
      ALOG("error getting the output frame count.");
      return CUBEB_ERROR;
    }
    status = ctx->klass.get_output_latency((uint32_t*)&latency, AUDIO_STREAM_TYPE_MUSIC);
    if (status) {
      ALOG("error getting the output frame count.");
      return CUBEB_ERROR;
    }
    status = ctx->klass.get_output_samplingrate(&samplerate, AUDIO_STREAM_TYPE_MUSIC);
    if (status) {
      ALOG("error getting the output frame count.");
      return CUBEB_ERROR;
    }

    




    min_buffer_count = latency / ((1000 * frame_count) / samplerate);
    min_buffer_count = min_buffer_count < 2 ? min_buffer_count : 2;
    *min_frame_count = (frame_count * params->rate * min_buffer_count) / samplerate;
    return CUBEB_OK;
  }
  
  status = ctx->klass.get_min_frame_count(min_frame_count, AUDIO_STREAM_TYPE_MUSIC, params->rate);
  if (status != 0) {
    ALOG("error getting the min frame count");
    return CUBEB_ERROR;
  }
  return CUBEB_OK;
}

int
audiotrack_init(cubeb ** context, char const * context_name)
{
  cubeb * ctx;
  struct AudioTrack* c;

  assert(context);
  *context = NULL;

  ctx = calloc(1, sizeof(*ctx));
  assert(ctx);

  



  ctx->library = dlopen("libmedia.so", RTLD_LAZY);
  if (!ctx->library) {
    ALOG("dlopen error: %s.", dlerror());
    free(ctx);
    return CUBEB_ERROR;
  }

  
  DLSYM_DLERROR("_ZN7android10AudioTrackC1EijiiijPFviPvS1_ES1_ii", ctx->klass.ctor, ctx->library);
  if (!ctx->klass.ctor) {
    DLSYM_DLERROR("_ZN7android10AudioTrackC1EijiiijPFviPvS1_ES1_i", ctx->klass.ctor_froyo, ctx->library);
    assert(ctx->klass.ctor_froyo);
  }
  DLSYM_DLERROR("_ZN7android10AudioTrackD1Ev", ctx->klass.dtor, ctx->library);

  DLSYM_DLERROR("_ZNK7android10AudioTrack7latencyEv", ctx->klass.latency, ctx->library);
  DLSYM_DLERROR("_ZNK7android10AudioTrack9initCheckEv", ctx->klass.check, ctx->library);

  
  if (audiotrack_version_is_froyo(ctx)) {
    DLSYM_DLERROR("_ZN7android11AudioSystem19getOutputFrameCountEPii", ctx->klass.get_output_frame_count, ctx->library);
    DLSYM_DLERROR("_ZN7android11AudioSystem16getOutputLatencyEPji", ctx->klass.get_output_latency, ctx->library);
    DLSYM_DLERROR("_ZN7android11AudioSystem21getOutputSamplingRateEPii", ctx->klass.get_output_samplingrate, ctx->library);
  } else {
    DLSYM_DLERROR("_ZN7android10AudioTrack16getMinFrameCountEPi19audio_stream_type_tj", ctx->klass.get_min_frame_count, ctx->library);
  }

  DLSYM_DLERROR("_ZN7android10AudioTrack5startEv", ctx->klass.start, ctx->library);
  DLSYM_DLERROR("_ZN7android10AudioTrack5pauseEv", ctx->klass.pause, ctx->library);
  DLSYM_DLERROR("_ZN7android10AudioTrack11getPositionEPj", ctx->klass.get_position, ctx->library);
  DLSYM_DLERROR("_ZN7android10AudioTrack17setMarkerPositionEj", ctx->klass.set_marker_position, ctx->library);

  
  c = &ctx->klass;
  if(!((c->ctor || c->ctor_froyo) && 
     c->dtor && c->latency && c->check &&
     
     ((c->get_output_frame_count && c->get_output_latency && c->get_output_samplingrate) ||
      c->get_min_frame_count) &&
     c->start && c->pause && c->get_position && c->set_marker_position)) {
    ALOG("Could not find all the symbols we need.");
    audiotrack_destroy(ctx);
    return CUBEB_ERROR;
  }

  ctx->ops = &audiotrack_ops;

  *context = ctx;

  return CUBEB_OK;
}

char const *
audiotrack_get_backend_id(cubeb * context)
{
  return "audiotrack";
}

void
audiotrack_destroy(cubeb * context)
{
  assert(context);

  dlclose(context->library);

  free(context);
}

int
audiotrack_stream_init(cubeb * ctx, cubeb_stream ** stream, char const * stream_name,
                       cubeb_stream_params stream_params, unsigned int latency,
                       cubeb_data_callback data_callback,
                       cubeb_state_callback state_callback,
                       void * user_ptr)
{
  struct cubeb_stream * stm;
  int32_t channels;
  int32_t min_frame_count;

  assert(ctx && stream);

  if (stream_params.format == CUBEB_SAMPLE_FLOAT32LE ||
      stream_params.format == CUBEB_SAMPLE_FLOAT32BE) {
    return CUBEB_ERROR_INVALID_FORMAT;
  }

  if (audiotrack_get_min_frame_count(ctx, &stream_params, &min_frame_count)) {
    return CUBEB_ERROR;
  }

  stm = calloc(1, sizeof(*stm));
  assert(stm);

  stm->context = ctx;
  stm->data_callback = data_callback;
  stm->state_callback = state_callback;
  stm->user_ptr = user_ptr;
  stm->params = stream_params;

  stm->instance = calloc(SIZE_AUDIOTRACK_INSTANCE, 1);
  (*(uint32_t*)((intptr_t)stm->instance + SIZE_AUDIOTRACK_INSTANCE - 4)) = 0xbaadbaad;
  assert(stm->instance && "cubeb: EOM");

  if (audiotrack_version_is_froyo(ctx)) {
    channels = stm->params.channels == 2 ? AUDIO_CHANNEL_OUT_STEREO_Froyo : AUDIO_CHANNEL_OUT_MONO_Froyo;
  } else {
    channels = stm->params.channels == 2 ? AUDIO_CHANNEL_OUT_STEREO_ICS : AUDIO_CHANNEL_OUT_MONO_ICS;
  }

  if (audiotrack_version_is_froyo(ctx)) {
    ctx->klass.ctor_froyo(stm->instance,
                          AUDIO_STREAM_TYPE_MUSIC,
                          stm->params.rate,
                          AUDIO_FORMAT_PCM_16_BIT,
                          channels,
                          min_frame_count,
                          0,
                          audiotrack_refill,
                          stm,
                          0);
  } else {
    ctx->klass.ctor(stm->instance,
                    AUDIO_STREAM_TYPE_MUSIC,
                    stm->params.rate,
                    AUDIO_FORMAT_PCM_16_BIT,
                    channels,
                    min_frame_count,
                    0,
                    audiotrack_refill,
                    stm,
                    0,
                    0);
  }

  assert((*(uint32_t*)((intptr_t)stm->instance + SIZE_AUDIOTRACK_INSTANCE - 4)) == 0xbaadbaad);

  if (ctx->klass.check(stm->instance)) {
    ALOG("stream not initialized properly.");
    audiotrack_stream_destroy(stm);
    return CUBEB_ERROR;
  }

  *stream = stm;

  return CUBEB_OK;
}

void
audiotrack_stream_destroy(cubeb_stream * stream)
{
  assert(stream->context);

  stream->context->klass.dtor(stream->instance);

  free(stream->instance);
  stream->instance = NULL;
  free(stream);
}

int
audiotrack_stream_start(cubeb_stream * stream)
{
  assert(stream->instance);

  stream->context->klass.start(stream->instance);
  stream->state_callback(stream, stream->user_ptr, CUBEB_STATE_STARTED);

  return CUBEB_OK;
}

int
audiotrack_stream_stop(cubeb_stream * stream)
{
  assert(stream->instance);

  stream->context->klass.pause(stream->instance);
  stream->state_callback(stream, stream->user_ptr, CUBEB_STATE_STOPPED);

  return CUBEB_OK;
}

int
audiotrack_stream_get_position(cubeb_stream * stream, uint64_t * position)
{
  uint32_t p;

  assert(stream->instance && position);
  stream->context->klass.get_position(stream->instance, &p);
  *position = p;

  return CUBEB_OK;
}

static struct cubeb_ops const audiotrack_ops = {
  .init = audiotrack_init,
  .get_backend_id = audiotrack_get_backend_id,
  .destroy = audiotrack_destroy,
  .stream_init = audiotrack_stream_init,
  .stream_destroy = audiotrack_stream_destroy,
  .stream_start = audiotrack_stream_start,
  .stream_stop = audiotrack_stream_stop,
  .stream_get_position = audiotrack_stream_get_position
};
