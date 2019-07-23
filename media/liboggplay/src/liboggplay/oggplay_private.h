





































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
} OggPlayDecode;

typedef struct {
  OggPlayDecode   decoder;
  theora_state    video_handle;
  theora_info     video_info;
  theora_comment  video_comment;
  int             remaining_header_packets;
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
  kate_state      k;
  int             init;
#ifdef HAVE_TIGER
  int use_tiger;
  int overlay_dest;
  tiger_renderer *tr;
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
};

void
oggplay_set_data_callback_force(OggPlay *me, OggPlayDataCallback callback,
                void *user);

void
oggplay_take_out_trash(OggPlay *me, OggPlaySeekTrash *trash);

void
oggplay_seek_cleanup(OggPlay *me, ogg_int64_t milliseconds);

typedef struct {
  void (*init)(void *user_data);
  int (*callback)(OGGZ * oggz, ogg_packet * op, long serialno,
                                                          void * user_data);
  void (*shutdown)(void *user_data);
  int size;
} OggPlayCallbackFunctions;













#define OGGPLAY_TIME_INT_TO_FP(x) ((x) << 32)
#define OGGPLAY_TIME_FP_TO_INT(x) (((((x) >> 16) * 1000) >> 16) & 0xFFFFFFFF)



#define oggplay_malloc _ogg_malloc
#define oggplay_calloc _ogg_calloc
#define oggplay_realloc _ogg_realloc
#define oggplay_free _ogg_free

#include "oggplay_callback.h"
#include "oggplay_data.h"
#include "oggplay_buffer.h"

#endif
