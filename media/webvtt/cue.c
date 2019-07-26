


























#include "parser_internal.h"
#include "cue_internal.h"

WEBVTT_EXPORT webvtt_status
webvtt_create_cue( webvtt_cue **pcue )
{
  webvtt_cue *cue;
  if( !pcue ) {
    return WEBVTT_INVALID_PARAM;
  }
  cue = (webvtt_cue *)webvtt_alloc0( sizeof(*cue) );
  if( !cue ) {
    return WEBVTT_OUT_OF_MEMORY;
  }
  












  webvtt_ref( &cue->refs );
  webvtt_init_string( &cue->id );
  cue->from = 0xFFFFFFFFFFFFFFFF;
  cue->until = 0xFFFFFFFFFFFFFFFF;
  cue->snap_to_lines = 1;
  cue->settings.position = 50;
  cue->settings.size = 100;
  cue->settings.align = WEBVTT_ALIGN_MIDDLE;
  cue->settings.line = WEBVTT_AUTO;
  cue->settings.vertical = WEBVTT_HORIZONTAL;

  *pcue = cue;
  return WEBVTT_SUCCESS;
}

WEBVTT_EXPORT void
webvtt_ref_cue( webvtt_cue *cue )
{
  if( cue ) {
    webvtt_ref( &cue->refs );
  }
}

WEBVTT_EXPORT void
webvtt_release_cue( webvtt_cue **pcue )
{
  if( pcue && *pcue ) {
    webvtt_cue *cue = *pcue;
    *pcue = 0;
    if( webvtt_deref( &cue->refs ) == 0 ) {
      webvtt_release_string( &cue->id );
      webvtt_release_node( &cue->node_head );
      webvtt_free( cue );
    }
  }
}

WEBVTT_EXPORT int
webvtt_validate_cue( webvtt_cue *cue )
{
  if( cue ) {
    



    if( BAD_TIMESTAMP(cue->from) || BAD_TIMESTAMP(cue->until) ) {
      goto error;
    }

    if( cue->until <= cue->from ) {
      goto error;
    }

    



    return 1;
  }

error:
  return 0;
}
