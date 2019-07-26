


























#ifndef __WEBVTT_CUE_H__
# define __WEBVTT_CUE_H__
# include "util.h"
# include <webvtt/string.h>
# include <webvtt/node.h>

#if defined(__cplusplus) || defined(c_plusplus)
#define WEBVTT_CPLUSPLUS 1
extern "C" {
#endif

#define WEBVTT_AUTO (0xFFFFFFFF)

typedef enum
webvtt_vertical_type_t {
  WEBVTT_HORIZONTAL = 0,
  WEBVTT_VERTICAL_LR = 1,
  WEBVTT_VERTICAL_RL = 2
} webvtt_vertical_type;

typedef enum
webvtt_align_type_t {
  WEBVTT_ALIGN_START = 0,
  WEBVTT_ALIGN_MIDDLE,
  WEBVTT_ALIGN_END,

  WEBVTT_ALIGN_LEFT,
  WEBVTT_ALIGN_RIGHT
} webvtt_align_type;

typedef struct
webvtt_cue_settings_t {
  webvtt_vertical_type vertical;
  int line;
  webvtt_uint position;
  webvtt_uint size;
  webvtt_align_type align;
} webvtt_cue_settings;

typedef struct
webvtt_cue_t {
  



  struct webvtt_refcount_t refs;
  webvtt_uint flags;

  


  webvtt_timestamp from;
  webvtt_timestamp until;
  webvtt_cue_settings settings;
  webvtt_bool snap_to_lines;
  webvtt_string id;

  


  webvtt_node *node_head;
} webvtt_cue;

WEBVTT_EXPORT webvtt_status webvtt_create_cue( webvtt_cue **pcue );
WEBVTT_EXPORT void webvtt_ref_cue( webvtt_cue *cue );
WEBVTT_EXPORT void webvtt_release_cue( webvtt_cue **pcue );
WEBVTT_EXPORT int webvtt_validate_cue( webvtt_cue *cue );

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
