































#include "config.h"

#include "oggz_private.h"

int
oggz_stream_set_content (OGGZ * oggz, long serialno, int content)
{
  oggz_stream_t * stream;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  stream->content = content;

  return 0;
}

OggzStreamContent
oggz_stream_get_content (OGGZ * oggz, long serialno)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;
  
  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  return stream->content;
}

const char *
oggz_stream_get_content_type (OGGZ *oggz, long serialno)
{
  int content = oggz_stream_get_content(oggz, serialno);

  if (content == OGGZ_ERR_BAD_SERIALNO || content == OGGZ_ERR_BAD_OGGZ)
  {
    return NULL;
  }

  return oggz_auto_codec_ident[content].content_type;
} 

int
oggz_stream_get_numheaders (OGGZ * oggz, long serialno)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;
  
  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  return stream->numheaders;
}

int
oggz_set_preroll (OGGZ * oggz, long serialno, int preroll)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  stream->preroll = preroll;

  return 0;
}

int
oggz_get_preroll (OGGZ * oggz, long serialno)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  return stream->preroll;
}
