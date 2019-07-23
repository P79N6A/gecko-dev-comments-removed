































#include "config.h"

#include "oggz_private.h"

#include "oggz/oggz_stream.h"

static ogg_int64_t
oggz_metric_dirac (OGGZ * oggz, long serialno,
                   ogg_int64_t granulepos, void * user_data)
{
  oggz_stream_t * stream;
  ogg_int64_t iframe, pframe;
  ogg_uint32_t pt;
  ogg_uint16_t dist;
  ogg_uint16_t delay;
  ogg_int64_t dt;
  ogg_int64_t units;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return -1;

  iframe = granulepos >> stream->granuleshift;
  pframe = granulepos - (iframe << stream->granuleshift);
  pt = (iframe + pframe) >> 9;
  delay = pframe >> 9;
  dt = (ogg_int64_t)pt - delay;

  units = dt * stream->granulerate_d / stream->granulerate_n;

#ifdef DEBUG
  printf ("oggz_..._granuleshift: serialno %010lu Got frame or field %lld (%lld + %lld): %lld units\n",
	  serialno, dt, iframe, pframe, units);
#endif

  return units;
}

static ogg_int64_t
oggz_metric_default_granuleshift (OGGZ * oggz, long serialno,
				  ogg_int64_t granulepos, void * user_data)
{
  oggz_stream_t * stream;
  ogg_int64_t iframe, pframe;
  ogg_int64_t units;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return -1;

  iframe = granulepos >> stream->granuleshift;
  pframe = granulepos - (iframe << stream->granuleshift);
  granulepos = iframe + pframe;
  if (granulepos > 0) granulepos -= stream->first_granule;

  units = granulepos * stream->granulerate_d / stream->granulerate_n;

#ifdef DEBUG
  printf ("oggz_..._granuleshift: serialno %010lu Got frame %lld (%lld + %lld): %lld units\n",
	  serialno, granulepos, iframe, pframe, units);
#endif

  return units;
}

static ogg_int64_t
oggz_metric_default_linear (OGGZ * oggz, long serialno, ogg_int64_t granulepos,
			    void * user_data)
{
  oggz_stream_t * stream;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return -1;

  return (stream->granulerate_d * granulepos / stream->granulerate_n);
}

static int
oggz_metric_update (OGGZ * oggz, long serialno)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  

  if (stream->granulerate_n == 0) {
    stream->granulerate_n= 1;
    stream->granulerate_d = 0;
  }

  if (stream->granuleshift == 0) {
    return oggz_set_metric_internal (oggz, serialno,
				     oggz_metric_default_linear,
				     NULL, 1);
  } else if (oggz_stream_get_content (oggz, serialno) == OGGZ_CONTENT_DIRAC) {
    return oggz_set_metric_internal (oggz, serialno,
				     oggz_metric_dirac,
				     NULL, 1);
  } else {
    return oggz_set_metric_internal (oggz, serialno,
				     oggz_metric_default_granuleshift,
				     NULL, 1);
  }
}

int
oggz_set_granuleshift (OGGZ * oggz, long serialno, int granuleshift)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  stream->granuleshift = granuleshift;

  return oggz_metric_update (oggz, serialno);
}

int
oggz_get_granuleshift (OGGZ * oggz, long serialno)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  return stream->granuleshift;
}

int
oggz_set_granulerate (OGGZ * oggz, long serialno,
		      ogg_int64_t granule_rate_numerator,
		      ogg_int64_t granule_rate_denominator)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  stream->granulerate_n = granule_rate_numerator;
  stream->granulerate_d = granule_rate_denominator;

  return oggz_metric_update (oggz, serialno);
}

int
oggz_get_granulerate (OGGZ * oggz, long serialno,
		      ogg_int64_t * granulerate_n,
		      ogg_int64_t * granulerate_d)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  *granulerate_n = stream->granulerate_n;
  *granulerate_d = stream->granulerate_d / OGGZ_AUTO_MULT;

  return 0;
}

int
oggz_set_first_granule (OGGZ * oggz, long serialno,
		        ogg_int64_t first_granule)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  stream->first_granule = first_granule;

  return oggz_metric_update (oggz, serialno);
}


int
oggz_set_metric_linear (OGGZ * oggz, long serialno,
			ogg_int64_t granule_rate_numerator,
			ogg_int64_t granule_rate_denominator)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  stream->granulerate_n = granule_rate_numerator;
  stream->granulerate_d = granule_rate_denominator;
  stream->granuleshift = 0;

  return oggz_metric_update (oggz, serialno);
}

