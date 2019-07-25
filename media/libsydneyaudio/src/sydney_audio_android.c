



































#include <stdlib.h>
#include <jni.h>
#include "sydney_audio.h"




struct AudioTrack {
  jclass    class;
  jmethodID constructor;
  jmethodID flush;
  jmethodID pause;
  jmethodID play;
  jmethodID setvol;
  jmethodID stop;
  jmethodID write;
  jmethodID getpos;
};

enum AudioTrackMode {
  MODE_STATIC = 0,
  MODE_STREAM = 1
};


enum AudioManagerStream {
  STREAM_VOICE_CALL = 0,
  STREAM_SYSTEM = 1,
  STREAM_RING = 2,
  STREAM_MUSIC = 3,
  STREAM_ALARM = 4,
  STREAM_NOTIFICATION = 5,
  STREAM_DTMF = 8
};


enum AudioFormatChannel {
  CHANNEL_OUT_MONO = 4,
  CHANNEL_OUT_STEREO = 12
};

enum AudioFormatEncoding {
  ENCODING_PCM_16BIT = 2,
  ENCODING_PCM_8BIT = 3
};

struct sa_stream {
  jobject output_unit;

  unsigned int rate;
  unsigned int channels;
  jclass at_class;
};

static struct AudioTrack at;
extern JNIEnv * GetJNIForThread();

static jclass
init_jni_bindings(JNIEnv *jenv) {
  jclass class =
    (*jenv)->NewGlobalRef(jenv,
                          (*jenv)->FindClass(jenv,
                                             "android/media/AudioTrack"));
  at.constructor = (*jenv)->GetMethodID(jenv, class, "<init>", "(IIIIII)V");
  at.flush       = (*jenv)->GetMethodID(jenv, class, "flush", "()V");
  at.pause       = (*jenv)->GetMethodID(jenv, class, "pause", "()V");
  at.play        = (*jenv)->GetMethodID(jenv, class, "play",  "()V");
  at.setvol      = (*jenv)->GetMethodID(jenv, class, "setStereoVolume",  "(FF)I");
  at.stop        = (*jenv)->GetMethodID(jenv, class, "stop",  "()V");
  at.write       = (*jenv)->GetMethodID(jenv, class, "write", "([BII)I");
  at.getpos      = (*jenv)->GetMethodID(jenv, class, "getPlaybackHeadPosition", "()I");

  return class;
};







int
sa_stream_create_pcm(
  sa_stream_t      ** _s,
  const char        * client_name,
  sa_mode_t           mode,
  sa_pcm_format_t     format,
  unsigned  int       rate,
  unsigned  int       channels
) {

  


  if (_s == NULL) {
    return SA_ERROR_INVALID;
  }
  *_s = NULL;

  if (mode != SA_MODE_WRONLY) {
    return SA_ERROR_NOT_SUPPORTED;
  }
  if (format != SA_PCM_FORMAT_S16_NE) {
    return SA_ERROR_NOT_SUPPORTED;
  }
  if (channels != 1 && channels != 2) {
    return SA_ERROR_NOT_SUPPORTED;
  }

  


  sa_stream_t *s;
  if ((s = malloc(sizeof(sa_stream_t))) == NULL) {
    return SA_ERROR_OOM;
  }

  s->output_unit = NULL;
  s->rate        = rate;
  s->channels    = channels;

  *_s = s;
  return SA_SUCCESS;
}


int
sa_stream_open(sa_stream_t *s) {

  if (s == NULL) {
    return SA_ERROR_NO_INIT;
  }
  if (s->output_unit != NULL) {
    return SA_ERROR_INVALID;
  }

  JNIEnv *jenv = GetJNIForThread();
  if (!jenv)
    return SA_ERROR_NO_DEVICE;

  if ((*jenv)->PushLocalFrame(jenv, 4)) {
    return SA_ERROR_OOM;
  }

  s->at_class = init_jni_bindings(jenv);

  jobject obj =
    (*jenv)->NewObject(jenv, s->at_class, at.constructor,
                       STREAM_MUSIC,
                       s->rate,
                       s->channels == 1 ?
                         CHANNEL_OUT_MONO : CHANNEL_OUT_STEREO,
                       ENCODING_PCM_16BIT,
                       (s->channels * s->rate) / 2,
                       MODE_STREAM);

  if (!obj) {
    (*jenv)->DeleteGlobalRef(jenv, s->at_class);
    (*jenv)->PopLocalFrame(jenv, NULL);
    return SA_ERROR_OOM;
  }

  s->output_unit = (*jenv)->NewGlobalRef(jenv, obj);
  (*jenv)->PopLocalFrame(jenv, NULL);

  return SA_SUCCESS;
}


int
sa_stream_destroy(sa_stream_t *s) {

  if (s == NULL) {
    return SA_ERROR_NO_INIT;
  }

  JNIEnv *jenv = GetJNIForThread();
  if (!jenv)
    return SA_SUCCESS;

  (*jenv)->DeleteGlobalRef(jenv, s->output_unit);
  (*jenv)->DeleteGlobalRef(jenv, s->at_class);
  free(s);

  return SA_SUCCESS;
}









int
sa_stream_write(sa_stream_t *s, const void *data, size_t nbytes) {

  if (s == NULL || s->output_unit == NULL) {
    return SA_ERROR_NO_INIT;
  }
  if (nbytes == 0) {
    return SA_SUCCESS;
  }
  JNIEnv *jenv = GetJNIForThread();
  if ((*jenv)->PushLocalFrame(jenv, 2)) {
    return SA_ERROR_OOM;
  }

  jbyteArray bytearray = (*jenv)->NewByteArray(jenv, nbytes);
  if (!bytearray) {
    (*jenv)->ExceptionClear(jenv);
    (*jenv)->PopLocalFrame(jenv, NULL);
    return SA_ERROR_OOM;
  }

  jbyte *byte = (*jenv)->GetByteArrayElements(jenv, bytearray, NULL);
  if (!byte) {
    (*jenv)->PopLocalFrame(jenv, NULL);
    return SA_ERROR_OOM;
  }

  memcpy(byte, data, nbytes);
  (*jenv)->ReleaseByteArrayElements(jenv, bytearray, byte, 0);
  jint retval = (*jenv)->CallIntMethod(jenv, s->output_unit, at.write,
                                       bytearray, 0, nbytes);

  (*jenv)->PopLocalFrame(jenv, NULL);

  return retval < 0 ? SA_ERROR_INVALID : SA_SUCCESS;
}








int
sa_stream_get_write_size(sa_stream_t *s, size_t *size) {

  if (s == NULL || s->output_unit == NULL) {
    return SA_ERROR_NO_INIT;
  }

  
  
  *size = (s->rate * s->channels) / 2;
  return SA_SUCCESS;
}


int
sa_stream_get_position(sa_stream_t *s, sa_position_t position, int64_t *pos) {

  if (s == NULL || s->output_unit == NULL) {
    return SA_ERROR_NO_INIT;
  }

  JNIEnv *jenv = GetJNIForThread();
  *pos  = (*jenv)->CallIntMethod(jenv, s->output_unit, at.getpos);
  *pos *= s->channels * 2;
  return SA_SUCCESS;
}


int
sa_stream_pause(sa_stream_t *s) {

  if (s == NULL || s->output_unit == NULL) {
    return SA_ERROR_NO_INIT;
  }

  JNIEnv *jenv = GetJNIForThread();
  (*jenv)->CallVoidMethod(jenv, s->output_unit, at.pause);
  return SA_SUCCESS;
}


int
sa_stream_resume(sa_stream_t *s) {

  if (s == NULL || s->output_unit == NULL) {
    return SA_ERROR_NO_INIT;
  }

  JNIEnv *jenv = GetJNIForThread();
  (*jenv)->CallVoidMethod(jenv, s->output_unit, at.play);
  return SA_SUCCESS;
}


int
sa_stream_drain(sa_stream_t *s)
{
  if (s == NULL || s->output_unit == NULL) {
    return SA_ERROR_NO_INIT;
  }

  JNIEnv *jenv = GetJNIForThread();
  (*jenv)->CallVoidMethod(jenv, s->output_unit, at.flush);
  return SA_SUCCESS;
}









int
sa_stream_set_volume_abs(sa_stream_t *s, float vol) {

  if (s == NULL || s->output_unit == NULL) {
    return SA_ERROR_NO_INIT;
  }

  JNIEnv *jenv = GetJNIForThread();
  (*jenv)->CallIntMethod(jenv, s->output_unit, at.setvol,
                         (jfloat)vol, (jfloat)vol);

  return SA_SUCCESS;
}






#define UNSUPPORTED(func)   func { return SA_ERROR_NOT_SUPPORTED; }

UNSUPPORTED(int sa_stream_create_opaque(sa_stream_t **s, const char *client_name, sa_mode_t mode, const char *codec))
UNSUPPORTED(int sa_stream_set_write_lower_watermark(sa_stream_t *s, size_t size))
UNSUPPORTED(int sa_stream_set_read_lower_watermark(sa_stream_t *s, size_t size))
UNSUPPORTED(int sa_stream_set_write_upper_watermark(sa_stream_t *s, size_t size))
UNSUPPORTED(int sa_stream_set_read_upper_watermark(sa_stream_t *s, size_t size))
UNSUPPORTED(int sa_stream_set_channel_map(sa_stream_t *s, const sa_channel_t map[], unsigned int n))
UNSUPPORTED(int sa_stream_set_xrun_mode(sa_stream_t *s, sa_xrun_mode_t mode))
UNSUPPORTED(int sa_stream_set_non_interleaved(sa_stream_t *s, int enable))
UNSUPPORTED(int sa_stream_set_dynamic_rate(sa_stream_t *s, int enable))
UNSUPPORTED(int sa_stream_set_driver(sa_stream_t *s, const char *driver))
UNSUPPORTED(int sa_stream_start_thread(sa_stream_t *s, sa_event_callback_t callback))
UNSUPPORTED(int sa_stream_stop_thread(sa_stream_t *s))
UNSUPPORTED(int sa_stream_change_device(sa_stream_t *s, const char *device_name))
UNSUPPORTED(int sa_stream_change_read_volume(sa_stream_t *s, const int32_t vol[], unsigned int n))
UNSUPPORTED(int sa_stream_change_write_volume(sa_stream_t *s, const int32_t vol[], unsigned int n))
UNSUPPORTED(int sa_stream_change_rate(sa_stream_t *s, unsigned int rate))
UNSUPPORTED(int sa_stream_change_meta_data(sa_stream_t *s, const char *name, const void *data, size_t size))
UNSUPPORTED(int sa_stream_change_user_data(sa_stream_t *s, const void *value))
UNSUPPORTED(int sa_stream_set_adjust_rate(sa_stream_t *s, sa_adjust_t direction))
UNSUPPORTED(int sa_stream_set_adjust_nchannels(sa_stream_t *s, sa_adjust_t direction))
UNSUPPORTED(int sa_stream_set_adjust_pcm_format(sa_stream_t *s, sa_adjust_t direction))
UNSUPPORTED(int sa_stream_set_adjust_watermarks(sa_stream_t *s, sa_adjust_t direction))
UNSUPPORTED(int sa_stream_get_mode(sa_stream_t *s, sa_mode_t *access_mode))
UNSUPPORTED(int sa_stream_get_codec(sa_stream_t *s, char *codec, size_t *size))
UNSUPPORTED(int sa_stream_get_pcm_format(sa_stream_t *s, sa_pcm_format_t *format))
UNSUPPORTED(int sa_stream_get_rate(sa_stream_t *s, unsigned int *rate))
UNSUPPORTED(int sa_stream_get_nchannels(sa_stream_t *s, int *nchannels))
UNSUPPORTED(int sa_stream_get_user_data(sa_stream_t *s, void **value))
UNSUPPORTED(int sa_stream_get_write_lower_watermark(sa_stream_t *s, size_t *size))
UNSUPPORTED(int sa_stream_get_read_lower_watermark(sa_stream_t *s, size_t *size))
UNSUPPORTED(int sa_stream_get_write_upper_watermark(sa_stream_t *s, size_t *size))
UNSUPPORTED(int sa_stream_get_read_upper_watermark(sa_stream_t *s, size_t *size))
UNSUPPORTED(int sa_stream_get_channel_map(sa_stream_t *s, sa_channel_t map[], unsigned int *n))
UNSUPPORTED(int sa_stream_get_xrun_mode(sa_stream_t *s, sa_xrun_mode_t *mode))
UNSUPPORTED(int sa_stream_get_non_interleaved(sa_stream_t *s, int *enabled))
UNSUPPORTED(int sa_stream_get_dynamic_rate(sa_stream_t *s, int *enabled))
UNSUPPORTED(int sa_stream_get_driver(sa_stream_t *s, char *driver_name, size_t *size))
UNSUPPORTED(int sa_stream_get_device(sa_stream_t *s, char *device_name, size_t *size))
UNSUPPORTED(int sa_stream_get_read_volume(sa_stream_t *s, int32_t vol[], unsigned int *n))
UNSUPPORTED(int sa_stream_get_write_volume(sa_stream_t *s, int32_t vol[], unsigned int *n))
UNSUPPORTED(int sa_stream_get_meta_data(sa_stream_t *s, const char *name, void*data, size_t *size))
UNSUPPORTED(int sa_stream_get_adjust_rate(sa_stream_t *s, sa_adjust_t *direction))
UNSUPPORTED(int sa_stream_get_adjust_nchannels(sa_stream_t *s, sa_adjust_t *direction))
UNSUPPORTED(int sa_stream_get_adjust_pcm_format(sa_stream_t *s, sa_adjust_t *direction))
UNSUPPORTED(int sa_stream_get_adjust_watermarks(sa_stream_t *s, sa_adjust_t *direction))
UNSUPPORTED(int sa_stream_get_state(sa_stream_t *s, sa_state_t *state))
UNSUPPORTED(int sa_stream_get_event_error(sa_stream_t *s, sa_error_t *error))
UNSUPPORTED(int sa_stream_get_event_notify(sa_stream_t *s, sa_notify_t *notify))
UNSUPPORTED(int sa_stream_read(sa_stream_t *s, void *data, size_t nbytes))
UNSUPPORTED(int sa_stream_read_ni(sa_stream_t *s, unsigned int channel, void *data, size_t nbytes))
UNSUPPORTED(int sa_stream_write_ni(sa_stream_t *s, unsigned int channel, const void *data, size_t nbytes))
UNSUPPORTED(int sa_stream_pwrite(sa_stream_t *s, const void *data, size_t nbytes, int64_t offset, sa_seek_t whence))
UNSUPPORTED(int sa_stream_pwrite_ni(sa_stream_t *s, unsigned int channel, const void *data, size_t nbytes, int64_t offset, sa_seek_t whence))
UNSUPPORTED(int sa_stream_get_read_size(sa_stream_t *s, size_t *size))
UNSUPPORTED(int sa_stream_get_volume_abs(sa_stream_t *s, float *vol))

const char *sa_strerror(int code) { return NULL; }

