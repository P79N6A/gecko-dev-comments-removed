





































#ifndef __OGGPLAY_PRIVATE_H__
#define __OGGPLAY_PRIVATE_H__

#ifdef HAVE_CONFIG_H 
#ifdef WIN32
#include "config_win32.h"
#else
#include <config.h>
#endif
#endif

#include <oggplay/oggplay.h>

#include <oggz/oggz.h>
#include <theora/theora.h>
#include <fishsound/fishsound.h>

#ifdef HAVE_KATE
#include <kate/kate.h>
#endif
#ifdef HAVE_TIGER
#include <tiger/tiger.h>
#endif

#ifdef WIN32

#ifdef HAVE_WINSOCK2
#include <winsock2.h>
#else
#include <winsock.h>
#endif
#endif

#ifdef OS2
#define INCL_DOSSEMAPHORES
#define INCL_DOSPROCESS
#include <os2.h>
#endif


#include "std_semaphore.h"






struct _OggPlayDataHeader {
  int                         lock;
  struct _OggPlayDataHeader * next;
  ogg_int64_t                 presentation_time;
  ogg_int64_t                 samples_in_record;
  int                         has_been_presented;
};

typedef struct {
  OggPlayDataHeader   header;
  OggPlayVideoData    data;
} OggPlayVideoRecord;

typedef struct {
  OggPlayDataHeader   header;
  OggPlayOverlayData  data;
} OggPlayOverlayRecord;

typedef struct {
  OggPlayDataHeader   header;
  void              * data;
} OggPlayAudioRecord;

typedef struct {
  OggPlayDataHeader   header;
  char              * data;
} OggPlayTextRecord;

struct _OggPlay;

typedef struct {
  void   ** buffer_list;
  void   ** buffer_mirror;
  int       buffer_size;
  int       last_filled;
  int       last_emptied;
  semaphore frame_sem;
} OggPlayBuffer;

struct _OggPlayCallbackInfo {
  OggPlayDataType       data_type;
  int                   available_records;
  int                   required_records;
  OggPlayStreamInfo     stream_info;
  OggPlayDataHeader  ** records;
  OggPlayBuffer       * buffer;
};





















typedef struct {
  long                  serialno;           
  int                   content_type;       
  const char          * content_type_name;  
  OggPlayDataType       decoded_type;       
  ogg_int64_t           granuleperiod;      
  ogg_int64_t           last_granulepos;    
  ogg_int64_t           offset;             
  ogg_int64_t           current_loc;        
  int                   active;             
  ogg_int64_t           final_granulepos;   
  struct _OggPlay     * player;             
  OggPlayDataHeader   * data_list;          
  OggPlayDataHeader   * end_of_data_list;   
  OggPlayDataHeader   * untimed_data_list;  
  OggPlayStreamInfo     stream_info;        
  int                   preroll;            
  short                 initialised;        
  int                   num_header_packets; 
} OggPlayDecode;

typedef struct {
  OggPlayDecode   decoder;
  theora_state    video_handle;
  theora_info     video_info;
  theora_comment  video_comment;
  int             granulepos_seen;
  int             frame_delta;
  int             y_width;
  int             y_height;
  int             y_stride;
  int             uv_width;
  int             uv_height;
  int             uv_stride;
  int             cached_keyframe;
  int             convert_to_rgb;
  int             swap_rgb;
} OggPlayTheoraDecode;

typedef struct {
  OggPlayDecode   decoder;
  FishSound     * sound_handle;
  FishSoundInfo   sound_info;
} OggPlayAudioDecode;

typedef struct {
  OggPlayDecode   decoder;
  int             granuleshift;
} OggPlayCmmlDecode;




typedef struct {
  OggPlayDecode   decoder;
  ogg_int64_t     presentation_time;
  ogg_int64_t     base_time;
} OggPlaySkeletonDecode;

typedef struct {
  OggPlayDecode   decoder;
#ifdef HAVE_KATE
  int             granuleshift;
  kate_state      k_state;
  kate_info       k_info;
  kate_comment    k_comment;
  int             init;
#ifdef HAVE_TIGER
  int use_tiger;
  int overlay_dest;
  tiger_renderer *tr;
  int default_width;
  int default_height;
  int swap_rgb;
#endif
#endif
} OggPlayKateDecode;

struct OggPlaySeekTrash;




typedef struct OggPlaySeekTrash {
  OggPlayDataHeader       * old_data;
  OggPlayBuffer           * old_buffer;
  struct OggPlaySeekTrash * next;
} OggPlaySeekTrash;




struct _OggPlay {
  OggPlayReader           * reader;                 
  OGGZ                    * oggz;                   
  OggPlayDecode          ** decode_data;            
  OggPlayCallbackInfo     * callback_info;          
  int                       num_tracks;             
  int                       all_tracks_initialised; 
  ogg_int64_t               callback_period;        
  OggPlayDataCallback     * callback;               
  void                    * callback_user_ptr;      
  ogg_int64_t               target;                 
  int                       active_tracks;          
  volatile OggPlayBuffer  * buffer;                 
  ogg_int64_t               presentation_time;      
  OggPlaySeekTrash        * trash;                  
  int                       shutdown;               
  int                       pt_update_valid;        
  ogg_int64_t               duration;	              
  int                       max_video_frame_pixels; 
};

void
oggplay_set_data_callback_force(OggPlay *me, OggPlayDataCallback callback,
                                void *user);

void
oggplay_take_out_trash(OggPlay *me, OggPlaySeekTrash *trash);

OggPlayErrorCode
oggplay_seek_cleanup(OggPlay *me, ogg_int64_t milliseconds);

typedef struct {
  void  (*init)     (void *user_data);
  int   (*callback) (OGGZ * oggz, ogg_packet * op, long serialno,
                     void * user_data);
  void  (*shutdown) (void *user_data);
  int   size;
} OggPlayCallbackFunctions;













#define OGGPLAY_TIME_INT_TO_FP(x) ((x) << 32)
#define OGGPLAY_TIME_FP_TO_INT(x) (((((x) >> 16) * 1000) >> 16) & 0xFFFFFFFF)



#define oggplay_malloc _ogg_malloc
#define oggplay_calloc _ogg_calloc
#define oggplay_realloc _ogg_realloc
#define oggplay_free _ogg_free





#define OGGPLAY_TYPE_HALF_MAX_SIGNED(type) ((type)1 << (sizeof(type)*8-2))
#define OGGPLAY_TYPE_MAX_SIGNED(type) (OGGPLAY_TYPE_HALF_MAX_SIGNED(type) - 1 + OGGPLAY_TYPE_HALF_MAX_SIGNED(type))
#define OGGPLAY_TYPE_MIN_SIGNED(type) (-1 - OGGPLAY_TYPE_MAX_SIGNED(type))
#define OGGPLAY_TYPE_MIN(type) ((type)-1 < 1?OGGPLAY_TYPE_MIN_SIGNED(type):(type)0)
#define OGGPLAY_TYPE_MAX(type) ((type)~OGGPLAY_TYPE_MIN(type))

static inline int
oggplay_check_add_overflow (size_t a, long b, size_t* r) {  
  
  if (sizeof(size_t) < sizeof(long)) {
    
    if
    (
      (b < 0) ?
        ((OGGPLAY_TYPE_MAX(size_t)+b >= 0) ? 0 : 1) :
        ((OGGPLAY_TYPE_MAX(size_t)-b >= 0) ? 0 : 1)
    )
    {
      return E_OGGPLAY_TYPE_OVERFLOW;
    }
  } 
  
  if
  (
    (b < 0) ?
      ((OGGPLAY_TYPE_MIN(size_t)-b <= a) ? 0 : 1) :
      ((OGGPLAY_TYPE_MAX(size_t)-b >= a) ? 0 : 1)
  )
  {
    return E_OGGPLAY_TYPE_OVERFLOW;
  }
  
  
  if (r != NULL)
    *r = a+b;
  
  return 0;
}

static inline int
oggplay_mul_signed_overflow_generic(long a, long b, long *re) {  
  long _a, _b, ah, bh, x, y, r = 0;
  int sign = 1;
  
  _a = a;
  _b = b;
  ah = _a >> (sizeof(long)*4);
  bh = _b >> (sizeof(long)*4);
  
  if (a < 0) {
    _a = -_a;
    if (_a < 0) {
      if (_b == 0 || _b == 1) {
        r = _a*_b;
        goto ok;
      } else {
        goto overflow;
      }
    }
    sign = -sign;
    ah = _a >> (sizeof(long)*4);
  }

  if (_b < 0) {
    _b = -_b;
    if (_b < 0) {
      if (_a == 0 || (_a == 1 && sign == 1)) {
        r = _a*_b;
        goto ok;
      } else {
        goto overflow;
      }
    }
    sign = -sign;
    bh = _b >> (sizeof(long)*4);
  }

  if (ah != 0 && bh != 0) {
    goto overflow;
  }
  
  if (ah == 0 && bh == 0) {
    r = _a*_b;
    if (r < 0)
      goto overflow;
    
    goto ok;
  }
  
  if (_a < _b) {
    x = _a;
    _a = _b;
    _b = x;
    ah = bh;
  }

  y = ah*_b;
  if (y >= (1L << (sizeof(long)*4 - 1)))
    goto overflow;
  _a &= (1L << sizeof(long)*4) - 1;
  x = _a*_b;
  if (x < 0)
    goto overflow;
  x += (y << sizeof(long)*4);
  if (x < 0)
    goto overflow;
          
ok:
  if (re != NULL) {
    *re = sign*r;
  }
  
  return 0;
  
overflow:
  return E_OGGPLAY_TYPE_OVERFLOW;
}

static inline int 
oggplay_mul_signed_overflow(long a, long b, long *r) {
  if (sizeof(long) > 4) {
    return oggplay_mul_signed_overflow_generic (a, b, r);
  } else {
    ogg_int64_t c = (ogg_int64_t) a*b;

    
    if
    (
      (c < 1) ?
        ((OGGPLAY_TYPE_MIN (long) > c) ? 1 : 0) :
        ((OGGPLAY_TYPE_MAX (long) < c) ? 1 : 0)
    ) 
    {
      return E_OGGPLAY_TYPE_OVERFLOW;
    }
    
    if (r != NULL) {
      *r = (long)c;
    }
    return 0; 
  }
}

#include "oggplay_callback.h"
#include "oggplay_data.h"
#include "oggplay_buffer.h"

#endif
