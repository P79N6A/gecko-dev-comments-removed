


























#ifndef __INTERN_CUE_H__
# define __INTERN_CUE_H__
# include <webvtt/cue.h>




enum {
  CUE_HAVE_VERTICAL = (1 << 0),
  CUE_HAVE_SIZE = (1 << 1),
  CUE_HAVE_POSITION = (1 << 2),
  CUE_HAVE_LINE = (1 << 3),
  CUE_HAVE_ALIGN = (1 << 4),

  CUE_HAVE_SETTINGS = (CUE_HAVE_VERTICAL | CUE_HAVE_SIZE
    | CUE_HAVE_POSITION | CUE_HAVE_LINE | CUE_HAVE_ALIGN),

  CUE_HAVE_CUEPARAMS = 0x40000000,
  CUE_HAVE_ID = 0x80000000,
  CUE_HEADER_MASK = CUE_HAVE_CUEPARAMS|CUE_HAVE_ID,
};

static webvtt_bool
cue_is_incomplete( const webvtt_cue *cue ) {
  return !cue || ( cue->flags & CUE_HEADER_MASK ) == CUE_HAVE_ID;
}

#endif
