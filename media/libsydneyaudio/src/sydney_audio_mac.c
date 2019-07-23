


































#include <pthread.h>
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>
#include "sydney_audio.h"




















typedef struct sa_buf sa_buf;
struct sa_buf {
  unsigned int      size;
  unsigned int      start;
  unsigned int      end;
  sa_buf          * next;
  unsigned char     data[0];
};

struct sa_stream {
  AudioUnit         output_unit;
  pthread_mutex_t   mutex;
  bool              playing;
  int64_t           bytes_played;
  int64_t           total_bytes_played;

  
  unsigned int      rate;
  unsigned int      n_channels;
  unsigned int      bytes_per_ch;

  
  sa_buf          * bl_head;
  sa_buf          * bl_tail;
  int               n_bufs;
};







#define BUF_SIZE    (2 * 44100 * 4)
#define BUF_LIMIT   5

#if BUF_LIMIT < 2
#error BUF_LIMIT must be at least 2!
#endif


static OSStatus audio_callback(void *arg, AudioUnitRenderActionFlags *action_flags,
  const AudioTimeStamp *time_stamp, UInt32 bus_num, UInt32 n_frames, AudioBufferList *data);

static sa_buf *new_buffer(void);








int
sa_stream_create_pcm(
  sa_stream_t      ** _s,
  const char        * client_name,
  sa_mode_t           mode,
  sa_pcm_format_t     format,
  unsigned  int       rate,
  unsigned  int       n_channels
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

  


  sa_stream_t   * s;
  if ((s = malloc(sizeof(sa_stream_t))) == NULL) {
    return SA_ERROR_OOM;
  }
  if ((s->bl_head = new_buffer()) == NULL) {
    free(s);
    return SA_ERROR_OOM;
  }
  if (pthread_mutex_init(&s->mutex, NULL) != 0) {
    free(s->bl_head);
    free(s);
    return SA_ERROR_SYSTEM;
  }

  s->output_unit  = NULL;
  s->playing      = FALSE;
  s->bytes_played = 0;
  s->total_bytes_played = 0;
  s->rate         = rate;
  s->n_channels   = n_channels;
  s->bytes_per_ch = 2;
  s->bl_tail      = s->bl_head;
  s->n_bufs       = 1;

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

  


  ComponentDescription desc;
  desc.componentType         = kAudioUnitType_Output;
  desc.componentSubType      = kAudioUnitSubType_DefaultOutput;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags        = 0;
  desc.componentFlagsMask    = 0;

  Component comp = FindNextComponent(NULL, &desc);
  if (comp == NULL) {
    return SA_ERROR_NO_DEVICE;
  }

  if (OpenAComponent(comp, &s->output_unit) != noErr) {
    return SA_ERROR_NO_DEVICE;
  }

  


  AURenderCallbackStruct input;
  input.inputProc       = audio_callback;
  input.inputProcRefCon = s;
  if (AudioUnitSetProperty(s->output_unit, kAudioUnitProperty_SetRenderCallback,
      kAudioUnitScope_Input, 0, &input, sizeof(input)) != 0) {
    return SA_ERROR_SYSTEM;
  }

  














  AudioStreamBasicDescription fmt;
  fmt.mFormatID         = kAudioFormatLinearPCM;
  fmt.mFormatFlags      = kLinearPCMFormatFlagIsSignedInteger |
#ifdef __BIG_ENDIAN__
                          kLinearPCMFormatFlagIsBigEndian |
#endif
                          kLinearPCMFormatFlagIsPacked;
  fmt.mSampleRate       = s->rate;
  fmt.mChannelsPerFrame = s->n_channels;
  fmt.mBitsPerChannel   = s->bytes_per_ch * 8;
  fmt.mFramesPerPacket  = 1;  
  fmt.mBytesPerFrame    = fmt.mChannelsPerFrame * fmt.mBitsPerChannel / 8;
  fmt.mBytesPerPacket   = fmt.mBytesPerFrame * fmt.mFramesPerPacket;

  






  if (AudioUnitSetProperty(s->output_unit, kAudioUnitProperty_StreamFormat,
      kAudioUnitScope_Input, 0, &fmt, sizeof(AudioStreamBasicDescription)) != 0) {
    return SA_ERROR_NOT_SUPPORTED;
  }

  if (AudioUnitInitialize(s->output_unit) != 0) {
    return SA_ERROR_SYSTEM;
  }

  return SA_SUCCESS;
}


int
sa_stream_destroy(sa_stream_t *s) {

  if (s == NULL) {
    return SA_SUCCESS;
  }

  









  int result = SA_SUCCESS;
  if (s->output_unit != NULL) {
    if (s->playing && AudioOutputUnitStop(s->output_unit) != 0) {
      result = SA_ERROR_SYSTEM;
    }
    if (AudioUnitUninitialize(s->output_unit) != 0) {
      result = SA_ERROR_SYSTEM;
    }
    if (CloseComponent(s->output_unit) != noErr) {
      result = SA_ERROR_SYSTEM;
    }
  }

  


  if (pthread_mutex_destroy(&s->mutex) != 0) {
    result = SA_ERROR_SYSTEM;
  }
  while (s->bl_head != NULL) {
    sa_buf  * next = s->bl_head->next;
    free(s->bl_head);
    s->bl_head = next;
  }
  free(s);

  return result;
}









int
sa_stream_write(sa_stream_t *s, const void *data, size_t nbytes) {

  if (s == NULL || s->output_unit == NULL) {
    return SA_ERROR_NO_INIT;
  }
  if (nbytes == 0) {
    return SA_SUCCESS;
  }

  pthread_mutex_lock(&s->mutex);

  


  int result = SA_SUCCESS;
  while (1) {
    unsigned int avail = s->bl_tail->size - s->bl_tail->end;

    if (nbytes <= avail) {

      



      memcpy(s->bl_tail->data + s->bl_tail->end, data, nbytes);
      s->bl_tail->end += nbytes;
      break;

    } else {

      



      memcpy(s->bl_tail->data + s->bl_tail->end, data, avail);
      s->bl_tail->end += avail;
      data = ((unsigned char *)data) + avail;
      nbytes -= avail;

      




      if (nbytes > 0 && s->n_bufs == BUF_LIMIT) {
#ifdef TIMING_TRACE
        printf("#");  
#endif
        if (!s->playing) {
          





          printf("Too much audio data received before audio device enabled!\n");
          result = SA_ERROR_SYSTEM;
          break;
        }
        while (s->n_bufs == BUF_LIMIT) {
          pthread_mutex_unlock(&s->mutex);
          struct timespec ts = {0, 1000000};
          nanosleep(&ts, NULL);
          pthread_mutex_lock(&s->mutex);
        }
      }

      


      if ((s->bl_tail->next = new_buffer()) == NULL) {
        result = SA_ERROR_OOM;
        break;
      }
      s->n_bufs++;
      s->bl_tail = s->bl_tail->next;
    
    } 

  } 

  pthread_mutex_unlock(&s->mutex);

  





  if (!s->playing) {
    if (AudioOutputUnitStart(s->output_unit) != 0) {
      return SA_ERROR_SYSTEM;
    }
    s->playing = TRUE;
  }

  return result;
}


static OSStatus
audio_callback(
  void                        * arg,
  AudioUnitRenderActionFlags  * action_flags,
  const AudioTimeStamp        * time_stamp,
  UInt32                        bus_num,
  UInt32                        n_frames,
  AudioBufferList             * data
) {

#ifdef TIMING_TRACE
  printf(".");  
#endif

  



  assert(data->mNumberBuffers == 1);

  sa_stream_t     * s = arg;

  pthread_mutex_lock(&s->mutex);

  unsigned char   * dst             = data->mBuffers[0].mData;
  unsigned int      bytes_per_frame = s->n_channels * s->bytes_per_ch;
  unsigned int      bytes_to_copy   = n_frames * bytes_per_frame;

  




  assert(time_stamp->mFlags & kAudioTimeStampSampleTimeValid);
  s->bytes_played = (int64_t)time_stamp->mSampleTime * bytes_per_frame;

  


  while (1) {
    assert(s->bl_head->start <= s->bl_head->end);
    unsigned int avail = s->bl_head->end - s->bl_head->start;

    if (avail >= bytes_to_copy) {

      


      memcpy(dst, s->bl_head->data + s->bl_head->start, bytes_to_copy);
      s->bl_head->start += bytes_to_copy;
      break;

    } else {

      


      memcpy(dst, s->bl_head->data + s->bl_head->start, avail);
      s->bl_head->start += avail;
      dst += avail;
      bytes_to_copy -= avail;

      




      sa_buf  * next = s->bl_head->next;
      if (next == NULL) {
#ifdef TIMING_TRACE
        printf("!");  
#endif
        memset(dst, 0, bytes_to_copy);
        break;
      }
      free(s->bl_head);
      s->bl_head = next;
      s->n_bufs--;

    } 

  } 

  pthread_mutex_unlock(&s->mutex);
  return noErr;
}









int
sa_stream_get_write_size(sa_stream_t *s, size_t *size) {

  if (s == NULL || s->output_unit == NULL) {
    return SA_ERROR_NO_INIT;
  }

  pthread_mutex_lock(&s->mutex);

  



  unsigned int avail = s->bl_tail->size - s->bl_tail->end;
  avail += (BUF_LIMIT - s->n_bufs) * BUF_SIZE;
  *size = avail;

  pthread_mutex_unlock(&s->mutex);
  return SA_SUCCESS;
}


int
sa_stream_get_position(sa_stream_t *s, sa_position_t position, int64_t *pos) {

  if (s == NULL || s->output_unit == NULL) {
    return SA_ERROR_NO_INIT;
  }
  if (position != SA_POSITION_WRITE_SOFTWARE) {
    return SA_ERROR_NOT_SUPPORTED;
  }

  pthread_mutex_lock(&s->mutex);
  *pos = s->total_bytes_played + s->bytes_played;
  pthread_mutex_unlock(&s->mutex);
  return SA_SUCCESS;
}


int
sa_stream_pause(sa_stream_t *s) {

  if (s == NULL || s->output_unit == NULL) {
    return SA_ERROR_NO_INIT;
  }

  





  if (AudioOutputUnitStop(s->output_unit) != 0) {
    return SA_ERROR_SYSTEM;
  }
  s->playing = FALSE;

  return SA_SUCCESS;
}


int
sa_stream_resume(sa_stream_t *s) {

  if (s == NULL || s->output_unit == NULL) {
    return SA_ERROR_NO_INIT;
  }

  pthread_mutex_lock(&s->mutex);
  



  s->total_bytes_played += s->bytes_played;
  s->bytes_played = 0;
  pthread_mutex_unlock(&s->mutex);

  





  if (AudioOutputUnitStart(s->output_unit) != 0) {
    return SA_ERROR_SYSTEM;
  }
  s->playing = TRUE;

  return SA_SUCCESS;
}


static sa_buf *
new_buffer(void) {
  sa_buf  * b = malloc(sizeof(sa_buf) + BUF_SIZE);
  if (b != NULL) {
    b->size  = BUF_SIZE;
    b->start = 0;
    b->end   = 0;
    b->next  = NULL;
  }
  return b;
}


int
sa_stream_drain(sa_stream_t *s)
{
  if (s == NULL || s->output_unit == NULL) {
    return SA_ERROR_NO_INIT;
  }

  if (!s->playing) {
    return SA_ERROR_INVALID;
  }

  while (1) {
    pthread_mutex_lock(&s->mutex);
    sa_buf  * b;
    size_t    used = 0;
    for (b = s->bl_head; b != NULL; b = b->next) {
      used += b->end - b->start;
    }
    pthread_mutex_unlock(&s->mutex);

    if (used == 0) {
      break;
    }

    struct timespec ts = {0, 1000000};
    nanosleep(&ts, NULL);
  }
  return SA_SUCCESS;
}









int
sa_stream_set_volume_abs(sa_stream_t *s, float vol) {

  if (s == NULL || s->output_unit == NULL) {
    return SA_ERROR_NO_INIT;
  }

  pthread_mutex_lock(&s->mutex);
  AudioUnitSetParameter(s->output_unit, kHALOutputParam_Volume,
      kAudioUnitParameterFlag_Output, 0, vol, 0);
  pthread_mutex_unlock(&s->mutex);
  return SA_SUCCESS;
}


int
sa_stream_get_volume_abs(sa_stream_t *s, float *vol) {

  if (s == NULL || s->output_unit == NULL) {
    return SA_ERROR_NO_INIT;
  }

  pthread_mutex_lock(&s->mutex);
  Float32 local_vol = 0;
  AudioUnitGetParameter(s->output_unit, kHALOutputParam_Volume,
      kAudioUnitParameterFlag_Output, 0, &local_vol);
  *vol = local_vol;
  pthread_mutex_unlock(&s->mutex);
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

const char *sa_strerror(int code) { return NULL; }

