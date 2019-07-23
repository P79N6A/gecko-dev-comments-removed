





































#ifdef WIN32
#include "config_win32.h"
#else
#include "config.h"
#endif

#if OGGZ_CONFIG_READ

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <ogg/ogg.h>

#include "oggz_compat.h"
#include "oggz_private.h"




#define CHUNKSIZE 4096






int
oggz_set_data_start (OGGZ * oggz, oggz_off_t offset)
{
  if (oggz == NULL) return -1;

  if (offset < 0) return -1;

  oggz->offset_data_begin = offset;

  return 0;
}

static oggz_off_t
oggz_tell_raw (OGGZ * oggz)
{
  oggz_off_t offset_at;

  offset_at = oggz_io_tell (oggz);

  return offset_at;
}





int
oggz_seek_reset_stream(void *data) {
  ((oggz_stream_t *)data)->last_granulepos = -1L;
  return 0;
}

static oggz_off_t
oggz_seek_raw (OGGZ * oggz, oggz_off_t offset, int whence)
{
  OggzReader  * reader = &oggz->x.reader;
  oggz_off_t    offset_at;

  if (oggz_io_seek (oggz, offset, whence) == -1) {
    return -1;
  }

  offset_at = oggz_io_tell (oggz);

  oggz->offset = offset_at;

  ogg_sync_reset (&reader->ogg_sync);

  oggz_vector_foreach(oggz->streams, oggz_seek_reset_stream);
  
  return offset_at;
}

static int
oggz_stream_reset (void * data)
{
  oggz_stream_t * stream = (oggz_stream_t *) data;

  if (stream->ogg_stream.serialno != -1) {
    ogg_stream_reset (&stream->ogg_stream);
  }

  return 0;
}

static void
oggz_reset_streams (OGGZ * oggz)
{
  oggz_vector_foreach (oggz->streams, oggz_stream_reset);
}

static long
oggz_reset_seek (OGGZ * oggz, oggz_off_t offset, ogg_int64_t unit, int whence)
{
  OggzReader * reader = &oggz->x.reader;

  oggz_off_t offset_at;

  offset_at = oggz_seek_raw (oggz, offset, whence);
  if (offset_at == -1) return -1;

  oggz->offset = offset_at;

#ifdef DEBUG
  printf ("reset to %" PRI_OGGZ_OFF_T "d\n", offset_at);
#endif

  if (unit != -1) reader->current_unit = unit;

  return offset_at;
}

static long
oggz_reset (OGGZ * oggz, oggz_off_t offset, ogg_int64_t unit, int whence)
{
  oggz_reset_streams (oggz);
  return oggz_reset_seek (oggz, offset, unit, whence);
}

int
oggz_purge (OGGZ * oggz)
{
  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  if (oggz->flags & OGGZ_WRITE) {
    return OGGZ_ERR_INVALID;
  }

  oggz_reset_streams (oggz);

  if (oggz->file && oggz_reset (oggz, oggz->offset, -1, SEEK_SET) < 0) {
    return OGGZ_ERR_SYSTEM;
  }

  return 0;
}









static oggz_off_t
oggz_get_next_page (OGGZ * oggz, ogg_page * og)
{
  OggzReader * reader = &oggz->x.reader;
  char * buffer;
  long bytes = 0, more;
  oggz_off_t page_offset = 0, ret;
  int found = 0;

  do {
    more = ogg_sync_pageseek (&reader->ogg_sync, og);

    if (more == 0) {
      page_offset = 0;

      buffer = ogg_sync_buffer (&reader->ogg_sync, CHUNKSIZE);
      if ((bytes = (long) oggz_io_read (oggz, buffer, CHUNKSIZE)) == 0) {
	if (oggz->file && feof (oggz->file)) {
#ifdef DEBUG_VERBOSE
	  printf ("get_next_page: feof (oggz->file), returning -2\n");
#endif
	  clearerr (oggz->file);
	  return -2;
	}
      }
      if (bytes == OGGZ_ERR_SYSTEM) {
	  
	  return -1;
      }

      if (bytes == 0) {
#ifdef DEBUG_VERBOSE
	printf ("get_next_page: bytes == 0, returning -2\n");
#endif
	return -2;
      }

      ogg_sync_wrote(&reader->ogg_sync, bytes);

    } else if (more < 0) {
#ifdef DEBUG_VERBOSE
      printf ("get_next_page: skipped %ld bytes\n", -more);
#endif
      page_offset -= more;
    } else {
#ifdef DEBUG_VERBOSE
      printf ("get_next_page: page has %ld bytes\n", more);
#endif
      found = 1;
    }

  } while (!found);

  
  if (bytes > 0) {
    oggz->offset = oggz_tell_raw (oggz) - bytes + page_offset;
  } else {
    
    oggz->offset += page_offset;
  }
  
  ret = oggz->offset + more;

  return ret;
}

static oggz_off_t
oggz_get_next_start_page (OGGZ * oggz, ogg_page * og)
{
  oggz_off_t page_offset;
  int found = 0;

  do {
    page_offset = oggz_get_next_page (oggz, og);

    






    

    if (page_offset <= 0 || ogg_page_granulepos(og) > -1)
      found = 1;
  }
  while (!found);

  return page_offset;
}

static oggz_off_t
oggz_get_prev_start_page (OGGZ * oggz, ogg_page * og,
			 ogg_int64_t * granule, long * serialno)
{
  oggz_off_t offset_at, offset_start;
  oggz_off_t page_offset, found_offset = 0;
  ogg_int64_t unit_at;
  ogg_int64_t granule_at = -1;

#if 0
  offset_at = oggz_tell_raw (oggz);
  if (offset_at == -1) return -1;
#else
  offset_at = oggz->offset;
#endif

  offset_start = offset_at;

  do {

    offset_start = offset_start - CHUNKSIZE;
    if (offset_start < 0) offset_start = 0;

    offset_start = oggz_seek_raw (oggz, offset_start, SEEK_SET);
    if (offset_start == -1) return -1;

#ifdef DEBUG

    printf ("get_prev_start_page: [A] offset_at: @%" PRI_OGGZ_OFF_T "d\toffset_start: @%" PRI_OGGZ_OFF_T "d\n",
	    offset_at, offset_start);

    printf ("get_prev_start_page: seeked to %" PRI_OGGZ_OFF_T "d\n", offset_start);
#endif

    page_offset = 0;

    do {
      page_offset = oggz_get_next_start_page (oggz, og);
      if (page_offset == -1) {
#ifdef DEBUG
	printf ("get_prev_start_page: page_offset = -1\n");
#endif
	return -1;
      }
      if (page_offset == -2) {
#ifdef DEBUG
	printf ("*** get_prev_start_page: page_offset = -2\n");
#endif
	break;
      }

      granule_at = ogg_page_granulepos (og);

#ifdef DEBUG_VERBOSE
      printf ("get_prev_start_page: GOT page (%lld) @%" PRI_OGGZ_OFF_T "d\tat @%" PRI_OGGZ_OFF_T  "d\n",
	      granule_at, page_offset, offset_at);
#endif

      


      if (page_offset >= 0 && page_offset < offset_at) {
	found_offset = page_offset;
	*granule = granule_at;
	*serialno = ogg_page_serialno (og);
      }

    } while (page_offset >= 0 && page_offset < offset_at);

#ifdef DEBUG
    printf ("get_prev_start_page: [B] offset_at: @%" PRI_OGGZ_OFF_T "d\toffset_start: @%" PRI_OGGZ_OFF_T "d\n"
	    "found_offset: @%" PRI_OGGZ_OFF_T "d\tpage_offset: @%" PRI_OGGZ_OFF_T "d\n",
	    offset_at, offset_start, found_offset, page_offset);
#endif
    
    

  } while (found_offset == 0 && offset_start > 0);

  unit_at = oggz_get_unit (oggz, *serialno, *granule);
  offset_at = oggz_reset (oggz, found_offset, unit_at, SEEK_SET);

#ifdef DEBUG
    printf ("get_prev_start_page: [C] offset_at: @%" PRI_OGGZ_OFF_T "d\t"
	    "found_offset: @%" PRI_OGGZ_OFF_T "d\tunit_at: %lld\n",
	    offset_at, found_offset, unit_at);
#endif

  if (offset_at == -1) return -1;

  if (offset_at >= 0)
    return found_offset;
  else
    return -1;
}

static oggz_off_t
oggz_scan_for_page (OGGZ * oggz, ogg_page * og, ogg_int64_t unit_target,
		   oggz_off_t offset_begin, oggz_off_t offset_end)
{
  oggz_off_t offset_at, offset_next;
  oggz_off_t offset_prev = -1;
  ogg_int64_t granule_at;
  ogg_int64_t unit_at;
  long serialno;

#ifdef DEBUG
  printf (" SCANNING from %" PRI_OGGZ_OFF_T "d...", offset_begin);
#endif

  for ( ; ; ) {
    offset_at = oggz_seek_raw (oggz, offset_begin, SEEK_SET);
    if (offset_at == -1) return -1;

#ifdef DEBUG
    printf (" scan @%" PRI_OGGZ_OFF_T "d\n", offset_at);
#endif

    offset_next = oggz_get_next_start_page (oggz, og);

    if (offset_next < 0) {
      return offset_next;
    }

    if (offset_next == 0 && offset_begin != 0) {
#ifdef DEBUG
      printf (" ... scanned past EOF\n");
#endif
      return -1;
    }
    if (offset_next > offset_end) {
#ifdef DEBUG
      printf (" ... scanned to page %ld\n", (long)ogg_page_granulepos (og));
#endif

#if 0
      if (offset_prev != -1) {
	offset_at = oggz_seek_raw (oggz, offset_prev, SEEK_SET);
	if (offset_at == -1) return -1;

	offset_next = oggz_get_next_start_page (oggz, og);
	if (offset_next < 0) return offset_next;

	serialno = ogg_page_serialno (og);
	granule_at = ogg_page_granulepos (og);
	unit_at = oggz_get_unit (oggz, serialno, granule_at);

	return offset_at;
      } else {
	return -1;
      }
#else
      serialno = ogg_page_serialno (og);
      granule_at = ogg_page_granulepos (og);
      unit_at = oggz_get_unit (oggz, serialno, granule_at);
      
      return offset_at;
#endif
    }

    offset_at = offset_next;

    serialno = ogg_page_serialno (og);
    granule_at = ogg_page_granulepos (og);
    unit_at = oggz_get_unit (oggz, serialno, granule_at);

    if (unit_at < unit_target) {
#ifdef DEBUG
      printf (" scan: (%lld) < (%lld)\n", unit_at, unit_target);
#endif
      offset_prev = offset_next;
      offset_begin = offset_next+1;
    } else if (unit_at > unit_target) {
#ifdef DEBUG
      printf (" scan: (%lld) > (%lld)\n", unit_at, unit_target);
#endif
#if 0
      
      offset_at = oggz_seek_raw (oggz, offset_begin, SEEK_SET);
      if (offset_at == -1) return -1;

      offset_next = oggz_get_next_start_page (oggz, og);
      if (offset_next < 0) return offset_next;

      serialno = ogg_page_serialno (og);
      granule_at = ogg_page_granulepos (og);
      unit_at = oggz_get_unit (oggz, serialno, granule_at);

      break;
#else
      do {
        offset_at = oggz_get_prev_start_page(oggz, og, &granule_at, &serialno);
        unit_at = oggz_get_unit(oggz, serialno, granule_at);
      } while (unit_at > unit_target);
      return offset_at;
#endif
    } else if (unit_at == unit_target) {
#ifdef DEBUG
      printf (" scan: (%lld) == (%lld)\n", unit_at, unit_target);
#endif
      break;
    }
  }

  return offset_at;
}

#define GUESS_MULTIPLIER (1<<16)

static oggz_off_t
guess (ogg_int64_t unit_at, ogg_int64_t unit_target,
       ogg_int64_t unit_begin, ogg_int64_t unit_end,
       oggz_off_t offset_begin, oggz_off_t offset_end)
{
  ogg_int64_t guess_ratio;
  oggz_off_t offset_guess;

  if (unit_at == unit_begin) return offset_begin;

  if (unit_end != -1) {
    guess_ratio =
      GUESS_MULTIPLIER * (unit_target - unit_begin) /
      (unit_end - unit_begin);
  } else {
    guess_ratio =
      GUESS_MULTIPLIER * (unit_target - unit_begin) /
      (unit_at - unit_begin);
  }

#ifdef DEBUG
  printf ("oggz_seek::guess: guess_ratio %lld = (%lld - %lld) / (%lld - %lld)\n",
	  guess_ratio, unit_target, unit_begin, unit_at, unit_begin);
#endif
  
  offset_guess = offset_begin +
    (oggz_off_t)(((offset_end - offset_begin) * guess_ratio) /
		 GUESS_MULTIPLIER);
  
  return offset_guess;
}

static oggz_off_t
oggz_seek_guess (ogg_int64_t unit_at, ogg_int64_t unit_target,
		 ogg_int64_t unit_begin, ogg_int64_t unit_end,
		 oggz_off_t offset_at,
		 oggz_off_t offset_begin, oggz_off_t offset_end)
{
  oggz_off_t offset_guess;

  if (unit_at == unit_begin) {
    offset_guess = offset_begin + (offset_end - offset_begin)/2;
  } else if (unit_end == -1) {
    offset_guess = guess (unit_at, unit_target, unit_begin, unit_end,
			  offset_begin, offset_at);
  } else if (unit_end <= unit_begin) {
#ifdef DEBUG
    printf ("oggz_seek_guess: unit_end <= unit_begin (ERROR)\n");
#endif
    offset_guess = -1;
  } else {
    offset_guess = guess (unit_at, unit_target, unit_begin, unit_end,
			  offset_begin, offset_end);
  }

#ifdef DEBUG
    printf ("oggz_seek_guess: guessed %" PRI_OGGZ_OFF_T "d\n", offset_guess);
#endif

  return offset_guess;
}

static oggz_off_t
oggz_offset_end (OGGZ * oggz)
{
#ifndef WINCE
  int fd;
  struct stat statbuf;
#endif
  oggz_off_t offset_end = -1;

  if (oggz->file != NULL) {
#ifndef WINCE
    if ((fd = fileno (oggz->file)) == -1) {
      
      return -1;
    }

    if (fstat (fd, &statbuf) == -1) {
      
      return -1;
    }

    if (oggz_stat_regular (statbuf.st_mode)) {
      offset_end = statbuf.st_size;
#ifdef DEBUG
      printf ("oggz_offset_end: stat size %" PRI_OGGZ_OFF_T "d\n", offset_end);
#endif
    } else {
      

      
      
    }
#else
    int current = ftell(oggz->file);
    fseek (oggz->file, 0, SEEK_END);
    offset_end = ftell (oggz->file);
    fseek (oggz->file, current, SEEK_SET);
#endif
  } else {
    oggz_off_t offset_save;

    if (oggz->io == NULL || oggz->io->seek == NULL) {
      
      return -1;
    }

    
    offset_save = oggz_io_tell (oggz);
    if (oggz_io_seek (oggz, 0, SEEK_END) == -1) {
      return -1;
    }
    offset_end = oggz_io_tell (oggz);
    if (oggz_io_seek (oggz, offset_save, SEEK_SET) == -1) {
      return -1; 
    }
  }

  return offset_end;
}

ogg_int64_t
oggz_bounded_seek_set (OGGZ * oggz,
                       ogg_int64_t unit_target,
                       ogg_int64_t offset_begin,
                       ogg_int64_t offset_end)
{
  OggzReader * reader;
  oggz_off_t offset_orig, offset_at, offset_guess;
  oggz_off_t offset_next;
  ogg_int64_t granule_at;
  ogg_int64_t unit_at, unit_begin = -1, unit_end = -1, unit_last_iter = -1;
  long serialno;
  ogg_page * og;
  int hit_eof = 0;

  if (oggz == NULL) {
    return -1;
  }

  if (unit_target > 0 && !oggz_has_metrics (oggz)) {
#ifdef DEBUG
    printf ("oggz_bounded_seek_set: No metric defined, FAIL\n");
#endif
    return -1;
  }
  
  if (offset_end == -1 && (offset_end = oggz_offset_end (oggz)) == -1) {
#ifdef DEBUG
    printf ("oggz_bounded_seek_set: oggz_offset_end == -1, FAIL\n");
#endif
    return -1;
  }

  reader = &oggz->x.reader;

  if (unit_target == reader->current_unit) {
#ifdef DEBUG
    printf ("oggz_bounded_seek_set: unit_target == reader->current_unit, SKIP\n");
#endif
    return (long)reader->current_unit;
  }

  if (unit_target == 0) {
    offset_at = oggz_reset (oggz, oggz->offset_data_begin, 0, SEEK_SET);
    if (offset_at == -1) return -1;
    return 0;
  }

  offset_at = oggz_tell_raw (oggz);
  if (offset_at == -1) return -1;

  offset_orig = oggz->offset;

  unit_at = reader->current_unit;

  og = &oggz->current_page;

  if (unit_end == -1 && oggz_seek_raw (oggz, offset_end, SEEK_SET) >= 0) {
    ogg_int64_t granulepos;

    if (oggz_get_prev_start_page (oggz, og, &granulepos, &serialno) >= 0) {
      unit_end = oggz_get_unit (oggz, serialno, granulepos);
    }
  }

  if (unit_begin == -1 && oggz_seek_raw (oggz, offset_begin, SEEK_SET) >= 0) {
    ogg_int64_t granulepos;
    if (oggz_get_next_start_page (oggz, og) >= 0) {
      serialno = ogg_page_serialno (og);
      granulepos = ogg_page_granulepos (og);
      unit_begin = oggz_get_unit (oggz, serialno, granulepos);
    }
  }

  
  if (unit_target < unit_begin || unit_target > unit_end)
    return -1;

  
  if (unit_at > unit_begin && unit_at < unit_end) {
    if (unit_target < unit_at) {
      unit_end = unit_at;
      offset_end = offset_at;
    } else {
      unit_begin = unit_at;
      offset_begin = offset_at;
    }
  }

  og = &oggz->current_page;

  for ( ; ; ) {

    unit_last_iter = unit_at;
    hit_eof = 0;

#ifdef DEBUG
    printf ("oggz_bounded_seek_set: [A] want u%lld: (u%lld - u%lld) [@%" PRI_OGGZ_OFF_T "d - @%" PRI_OGGZ_OFF_T "d]\n",
	    unit_target, unit_begin, unit_end, offset_begin, offset_end);
#endif

    offset_guess = oggz_seek_guess (unit_at, unit_target,
				    unit_begin, unit_end,
				    offset_at,
				    offset_begin, offset_end);
    if (offset_guess == -1) break;

    if (offset_guess == offset_at) {
      
      break;
    }

    if (offset_guess > offset_end) {
      offset_guess = offset_end;
      offset_at = oggz_seek_raw (oggz, offset_guess, SEEK_SET);
      offset_next = oggz_get_prev_start_page (oggz, og, &granule_at, &serialno);
    } else {
      offset_at = oggz_seek_raw (oggz, offset_guess, SEEK_SET);
      offset_next = oggz_get_next_start_page (oggz, og);
      serialno = ogg_page_serialno (og);
      granule_at = ogg_page_granulepos (og);
    }

    unit_at = oggz_get_unit (oggz, serialno, granule_at);

#ifdef DEBUG
    printf ("oggz_bounded_seek_set: offset_next %" PRI_OGGZ_OFF_T "d\n", offset_next);
#endif
    if (unit_at == unit_last_iter) break;

#ifdef DEBUG
    printf ("oggz_bounded_seek_set: [D] want u%lld, got page u%lld @%" PRI_OGGZ_OFF_T "d g%lld\n",
	    unit_target, unit_at, offset_at, granule_at);
#endif

    if (unit_at < unit_target) {
      offset_begin = offset_at;
      unit_begin = unit_at;
      if (unit_end == unit_begin) break;
    } else if (unit_at > unit_target) {
      offset_end = offset_at-1;
      unit_end = unit_at;
      if (unit_end == unit_begin) break;
    } else {
      break;
    }
  }

  do {
    offset_at = oggz_get_prev_start_page (oggz, og, &granule_at, &serialno);
    unit_at = oggz_get_unit (oggz, serialno, granule_at);
  } while (unit_at > unit_target);

  if (offset_at < 0) {
    oggz_reset (oggz, offset_orig, -1, SEEK_SET);
    return -1;
  }

  offset_at = oggz_reset (oggz, offset_at, unit_at, SEEK_SET);
  if (offset_at == -1) return -1;

#ifdef DEBUG
  printf ("oggz_bounded_seek_set: FOUND (%lld)\n", unit_at);
#endif

  return (long)reader->current_unit;
}

static ogg_int64_t
oggz_seek_end (OGGZ * oggz, ogg_int64_t unit_offset)
{
  oggz_off_t offset_orig, offset_at, offset_end;
  ogg_int64_t granulepos;
  ogg_int64_t unit_end;
  long serialno;
  ogg_page * og;

  og = &oggz->current_page;

  offset_orig = oggz->offset;

  offset_at = oggz_seek_raw (oggz, 0, SEEK_END);
  if (offset_at == -1) return -1;

  offset_end = oggz_get_prev_start_page (oggz, og, &granulepos, &serialno);

  unit_end = oggz_get_unit (oggz, serialno, granulepos);

  if (offset_end < 0) {
    oggz_reset (oggz, offset_orig, -1, SEEK_SET);
    return -1;
  }

#ifdef DEBUG
  printf ("*** oggz_seek_end: found packet (%lld) at @%" PRI_OGGZ_OFF_T "d [%lld]\n",
	  unit_end, offset_end, granulepos);
#endif

  return oggz_bounded_seek_set (oggz, unit_end + unit_offset, 0, -1);
}

off_t
oggz_seek (OGGZ * oggz, oggz_off_t offset, int whence)
{
  OggzReader * reader;
  ogg_int64_t units = -1;

  if (oggz == NULL) return -1;

  if (oggz->flags & OGGZ_WRITE) {
    return -1;
  }

  if (offset == 0 && whence == SEEK_SET) units = 0;

  reader = &oggz->x.reader;

  if (!(offset == 0 && whence == SEEK_CUR)) {
    
    reader->current_unit = -1;
  }

  return (off_t)oggz_reset (oggz, offset, units, whence);
}

ogg_int64_t
oggz_seek_units (OGGZ * oggz, ogg_int64_t units, int whence)
{
  OggzReader * reader;

  ogg_int64_t r;

  if (oggz == NULL) {
#ifdef DEBUG
    printf ("oggz_seek_units: oggz NULL, FAIL\n");
#endif
    return -1;
  }

  if (oggz->flags & OGGZ_WRITE) {
#ifdef DEBUG
    printf ("oggz_seek_units: is OGGZ_WRITE, FAIL\n");
#endif
    return -1;
  }

  if (!oggz_has_metrics (oggz)) {
#ifdef DEBUG
    printf ("oggz_seek_units: !has_metrics, FAIL\n");
#endif
    return -1;
  }

  reader = &oggz->x.reader;

  switch (whence) {
  case SEEK_SET:
    r = oggz_bounded_seek_set (oggz, units, 0, -1);
    break;
  case SEEK_CUR:
    units += reader->current_unit;
    r = oggz_bounded_seek_set (oggz, units, 0, -1);
    break;
  case SEEK_END:
    r = oggz_seek_end (oggz, units);
    break;
  default:
    
    r = -1;
    break;
  }

  reader->current_granulepos = -1;
  return r;
}

long
oggz_seek_byorder (OGGZ * oggz, void * target)
{
  return -1;
}

long
oggz_seek_packets (OGGZ * oggz, long serialno, long packets, int whence)
{
  return -1;
}

#else

#include <ogg/ogg.h>
#include "oggz_private.h"

off_t
oggz_seek (OGGZ * oggz, oggz_off_t offset, int whence)
{
  return OGGZ_ERR_DISABLED;
}

long
oggz_seek_units (OGGZ * oggz, ogg_int64_t units, int whence)
{
  return OGGZ_ERR_DISABLED;
}

long
oggz_seek_byorder (OGGZ * oggz, void * target)
{
  return OGGZ_ERR_DISABLED;
}

long
oggz_seek_packets (OGGZ * oggz, long serialno, long packets, int whence)
{
  return OGGZ_ERR_DISABLED;
}

#endif



static int
is_any(ogg_int64_t* a, int n, ogg_int64_t val)
{
  int i;
  for (i=0; i<n; i++) {
    if (a[i] == val) {
      return 1;
    }
  }
  return 0;    
}



static int
find(long* a, int n, ogg_int64_t val)
{
  int i;
  for (i=0; i<n; i++) {
    if (a[i] == val) {
      return i;
    }
  }
  return -1;    
}



static ogg_int64_t
minimum(ogg_int64_t* a, int n) {
  ogg_int64_t m = 0x7FFFFFFFFFFFFFFF;
  int i;
  for (i=0; i<n; i++) {
    if (a[i] < m) {
      m = a[i];
    }
  }
  return m;
}

ogg_int64_t
oggz_keyframe_seek_set(OGGZ * oggz,
                       long* serial_nos,
                       int num_serialno,
                       ogg_int64_t unit_target,
                       ogg_int64_t offset_begin,
                       ogg_int64_t offset_end)
{
  oggz_off_t offset_at;
  oggz_off_t offset_next;
  ogg_int64_t granule_at;
  ogg_int64_t unit_at;
  ogg_int64_t key_granule_at, key_unit_at;
  long serialno;
  ogg_page * og;
  int granule_shift = 0, idx;
  ogg_int64_t* key_frames = 0;
  
  unit_at = oggz_bounded_seek_set(oggz,
                                  unit_target,
                                  offset_begin,
                                  offset_end);
  
  if (unit_at == -1)
    return -1;

  
  if (unit_at == 0)
    return 0; 

  
  offset_at = oggz->offset;

  key_frames = oggz_malloc(sizeof(ogg_int64_t) * num_serialno);
  if (!key_frames) {
    
    
    return unit_at;
  }
  memset(key_frames, -1, sizeof(ogg_int64_t) * num_serialno);

  
  og = &oggz->current_page;
  while (is_any(key_frames, num_serialno, -1)) {
    do {
      offset_next = oggz_get_prev_start_page (oggz, og, &granule_at, &serialno);
      if (offset_next <= 0 || granule_at == 0) {
        
        
        oggz_free(key_frames);
        offset_at = oggz_reset (oggz, offset_at, unit_at, SEEK_SET);
        return (offset_at == -1) ? -1 : unit_at;
      }
    } while (granule_at < 0);

    idx = find(serial_nos, num_serialno, serialno);
    if (idx == -1 || key_frames[idx] != -1)
      continue;

    granule_shift = oggz_get_granuleshift(oggz, serialno);
    key_granule_at = (granule_at >> granule_shift) << granule_shift;
    key_unit_at = oggz_get_unit(oggz, serialno, key_granule_at);

    if (key_unit_at < unit_target)
      key_frames[idx] = key_unit_at;
  }

  
  
  
  
  key_unit_at = minimum(key_frames, num_serialno);
  unit_at = oggz_bounded_seek_set(oggz,
                                  MAX((key_unit_at - 100), 0),
                                  offset_begin,
                                  offset_end);
  oggz_free(key_frames);

  return unit_at;
}
