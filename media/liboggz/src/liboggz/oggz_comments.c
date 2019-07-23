































#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> 
#ifndef WIN32
#include <strings.h>
#endif

#include <assert.h>

#include "oggz_private.h"
#include "oggz_vector.h"

#include "oggz/oggz_stream.h"



#ifdef WIN32                                                                   
#define strcasecmp _stricmp
#endif



#define MAX_COMMENT_LENGTH 0xFFFFFFFE
#define oggz_comment_clamp(c) MIN((c),MAX_COMMENT_LENGTH)

static size_t
oggz_comment_len (const char * s)
{
  size_t len;

  if (s == NULL) return 0;

  len = strlen (s);
  return oggz_comment_clamp(len);
}

static char *
oggz_strdup (const char * s)
{
  char * ret;
  if (s == NULL) return NULL;
  ret = oggz_malloc (oggz_comment_len(s) + 1);
  if (ret == NULL) return NULL;

  return strcpy (ret, s);
}

static char *
oggz_strdup_len (const char * s, size_t len)
{
  char * ret;
  if (s == NULL) return NULL;
  if (len == 0) return NULL;
  len = oggz_comment_clamp(len);
  ret = oggz_malloc (len + 1);
  if (!ret) return NULL;
  if (strncpy (ret, s, len) == NULL) {
    oggz_free (ret);
    return NULL;
  }

  ret[len] = '\0';
  return ret;
}

static char *
oggz_index_len (const char * s, char c, int len)
{
  int i;

  for (i = 0; *s && i < len; i++, s++) {
    if (*s == c) return (char *)s;
  }

  return NULL;
}





















#define readint(buf, base) (((buf[base+3]<<24)&0xff000000)| \
                           ((buf[base+2]<<16)&0xff0000)| \
                           ((buf[base+1]<<8)&0xff00)| \
  	           	    (buf[base]&0xff))
#define writeint(buf, base, val) buf[base+3]=(char)(((val)>>24)&0xff); \
                                 buf[base+2]=(char)(((val)>>16)&0xff); \
                                 buf[base+1]=(char)(((val)>>8)&0xff); \
                                 buf[base]=(char)((val)&0xff);




#define writeint24be(buf, base, val) buf[base]=(char)(((val)>>16)&0xff); \
                                     buf[base+1]=(char)(((val)>>8)&0xff); \
                                     buf[base+2]=(char)((val)&0xff);

static int
oggz_comment_validate_byname (const char * name, const char * value)
{
  const char * c;

  if (!name || !value) return 0;

  for (c = name; *c; c++) {
    if (*c < 0x20 || *c > 0x7D || *c == 0x3D) {
#ifdef DEBUG
      printf ("XXX char %c in %s invalid\n", *c, name);
#endif
      return 0;
    }
  }

  

  return 1;
}

static OggzComment *
oggz_comment_new (const char * name, const char * value)
{
  OggzComment * comment;

  if (!oggz_comment_validate_byname (name, value)) return NULL;
  

  comment = oggz_malloc (sizeof (OggzComment));
  if (comment == NULL) return NULL;

  comment->name = oggz_strdup (name);
  if (comment->name == NULL) {
    oggz_free (comment);
    return NULL;
  }

  comment->value = oggz_strdup (value);
  if (comment->value == NULL) {
    oggz_free (comment->name);
    oggz_free (comment);
    return NULL;
  }

  return comment;
}

static void
oggz_comment_free (OggzComment * comment)
{
  if (!comment) return;
  if (comment->name) oggz_free (comment->name);
  if (comment->value) oggz_free (comment->value);
  oggz_free (comment);
}

static int
oggz_comment_cmp (const OggzComment * comment1, const OggzComment * comment2)
{
  if (comment1 == comment2) return 1;
  if (!comment1 || !comment2) return 0;

  if (strcasecmp (comment1->name, comment2->name)) return 0;
  if (strcmp (comment1->value, comment2->value)) return 0;

  return 1;
}

static int
_oggz_comment_set_vendor (OGGZ * oggz, long serialno,
			  const char * vendor_string)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  if (stream->vendor) oggz_free (stream->vendor);

  if ((stream->vendor = oggz_strdup (vendor_string)) == NULL)
    return OGGZ_ERR_OUT_OF_MEMORY;

  return 0;
}



const char *
oggz_comment_get_vendor (OGGZ * oggz, long serialno)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return NULL;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return NULL;

  return stream->vendor;
}

int
oggz_comment_set_vendor (OGGZ * oggz, long serialno, const char * vendor_string)
{
  oggz_stream_t * stream;
  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL)
    stream = oggz_add_stream (oggz, serialno);
  if (stream == NULL)
    return OGGZ_ERR_OUT_OF_MEMORY;

  if (oggz->flags & OGGZ_WRITE) {
    if (OGGZ_CONFIG_WRITE) {

      return _oggz_comment_set_vendor (oggz, serialno, vendor_string);

    } else {
      return OGGZ_ERR_DISABLED;
    }
  } else {
    return OGGZ_ERR_INVALID;
  }
}


const OggzComment *
oggz_comment_first (OGGZ * oggz, long serialno)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return NULL;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return NULL;

  return oggz_vector_nth_p (stream->comments, 0);
}

const OggzComment *
oggz_comment_first_byname (OGGZ * oggz, long serialno, char * name)
{
  oggz_stream_t * stream;
  OggzComment * comment;
  int i;

  if (oggz == NULL) return NULL;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return NULL;

  if (name == NULL) return oggz_vector_nth_p (stream->comments, 0);

  if (!oggz_comment_validate_byname (name, ""))
    return NULL;

  for (i = 0; i < oggz_vector_size (stream->comments); i++) {
    comment = (OggzComment *) oggz_vector_nth_p (stream->comments, i);
    if (comment->name && !strcasecmp (name, comment->name))
      return comment;
  }

  return NULL;
}

const OggzComment *
oggz_comment_next (OGGZ * oggz, long serialno, const OggzComment * comment)
{
  oggz_stream_t * stream;
  int i;

  if (oggz == NULL || comment == NULL) return NULL;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return NULL;

  i = oggz_vector_find_index_p (stream->comments, comment);

  return oggz_vector_nth_p (stream->comments, i+1);
}

const OggzComment *
oggz_comment_next_byname (OGGZ * oggz, long serialno,
                          const OggzComment * comment)
{
  oggz_stream_t * stream;
  OggzComment * v_comment;
  int i;

  if (oggz == NULL || comment == NULL) return NULL;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return NULL;

  i = oggz_vector_find_index_p (stream->comments, comment);

  for (i++; i < oggz_vector_size (stream->comments); i++) {
    v_comment = (OggzComment *) oggz_vector_nth_p (stream->comments, i);
    if (v_comment->name && !strcasecmp (comment->name, v_comment->name))
      return v_comment;
  }

  return NULL;
}

static OggzComment *
_oggz_comment_add_byname (oggz_stream_t * stream, const char * name, const char * value)
{
  OggzComment * comment, * new_comment;
  int i;

  
  for (i = 0; i < oggz_vector_size (stream->comments); i++) {
    comment = (OggzComment *) oggz_vector_nth_p (stream->comments, i);
    if (comment->name && !strcasecmp (name, comment->name)) {
      if (comment->value == NULL) {
        if (value == NULL) return comment;
      } else if ((value && !strcmp (value, comment->value)) || 
                 (value == NULL && comment->value == NULL)) {
        return comment;
      }
    }
  }

  
  if ((new_comment = oggz_comment_new (name, value)) == NULL)
    return NULL;

  return oggz_vector_insert_p (stream->comments, new_comment);
}

int
oggz_comment_add (OGGZ * oggz, long serialno, const OggzComment * comment)
{
  oggz_stream_t * stream;
  OggzComment * new_comment;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL)
    stream = oggz_add_stream (oggz, serialno);
  if (stream == NULL)
    return OGGZ_ERR_OUT_OF_MEMORY;

  if (oggz->flags & OGGZ_WRITE) {
    if (OGGZ_CONFIG_WRITE) {

      if (!oggz_comment_validate_byname (comment->name, comment->value))
        return OGGZ_ERR_COMMENT_INVALID;

      if (_oggz_comment_add_byname (stream, comment->name, comment->value) == NULL)
        return OGGZ_ERR_OUT_OF_MEMORY;

      return 0;
    } else {
      return OGGZ_ERR_DISABLED;
    }
  } else {
    return OGGZ_ERR_INVALID;
  }
}

int
oggz_comment_add_byname (OGGZ * oggz, long serialno,
                         const char * name, const char * value)
{
  oggz_stream_t * stream;
  OggzComment * new_comment;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL)
    stream = oggz_add_stream (oggz, serialno);
  if (stream == NULL)
    return OGGZ_ERR_OUT_OF_MEMORY;

  if (oggz->flags & OGGZ_WRITE) {
    if (OGGZ_CONFIG_WRITE) {

      if (!oggz_comment_validate_byname (name, value))
        return OGGZ_ERR_COMMENT_INVALID;

      if (_oggz_comment_add_byname (stream, name, value) == NULL)
        return OGGZ_ERR_OUT_OF_MEMORY;

      return 0;
    } else {
      return OGGZ_ERR_DISABLED;
    }
  } else {
    return OGGZ_ERR_INVALID;
  }
}

int
oggz_comment_remove (OGGZ * oggz, long serialno, OggzComment * comment)
{
  oggz_stream_t * stream;
  OggzComment * v_comment;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  if (oggz->flags & OGGZ_WRITE) {
    if (OGGZ_CONFIG_WRITE) {
      v_comment = oggz_vector_find_p (stream->comments, comment);

      if (v_comment == NULL) return 0;

      oggz_vector_remove_p (stream->comments, v_comment);
      oggz_comment_free (v_comment);

      return 1;

    } else {
      return OGGZ_ERR_DISABLED;
    }
  } else {
    return OGGZ_ERR_INVALID;
  }
}

int
oggz_comment_remove_byname (OGGZ * oggz, long serialno, char * name)
{
  oggz_stream_t * stream;
  OggzComment * comment;
  int i, ret = 0;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  if (oggz->flags & OGGZ_WRITE) {
    if (OGGZ_CONFIG_WRITE) {
      for (i = 0; i < oggz_vector_size (stream->comments); i++) {
        comment = (OggzComment *) oggz_vector_nth_p (stream->comments, i);
        if (!strcasecmp (name, comment->name)) {
          oggz_comment_remove (oggz, serialno, comment);
          i--;
          ret++;
        }
      }
      return ret;
    } else {
      return OGGZ_ERR_DISABLED;
    }
  } else {
    return OGGZ_ERR_INVALID;
  }
}

int
oggz_comments_copy (OGGZ * src, long src_serialno,
                    OGGZ * dest, long dest_serialno)
{
  const OggzComment * comment;

  if (src == NULL || dest == NULL) return OGGZ_ERR_BAD_OGGZ;

  if (dest->flags & OGGZ_WRITE) {
    if (OGGZ_CONFIG_WRITE) {
      oggz_comment_set_vendor (dest, dest_serialno,
                               oggz_comment_get_vendor (src, src_serialno));

      for (comment = oggz_comment_first (src, src_serialno); comment;
           comment = oggz_comment_next (src, src_serialno, comment)) {
        oggz_comment_add (dest, dest_serialno, comment);
      }
    } else {
      return OGGZ_ERR_DISABLED;
    }
  } else {
    return OGGZ_ERR_INVALID;
  }

  return 0;
}


int
oggz_comments_init (oggz_stream_t * stream)
{
  stream->vendor = NULL;
  stream->comments = oggz_vector_new ();
  if (stream->comments == NULL) return -1;

  oggz_vector_set_cmp (stream->comments, (OggzCmpFunc) oggz_comment_cmp, NULL);

  return 0;
}

int
oggz_comments_free (oggz_stream_t * stream)
{
  oggz_vector_foreach (stream->comments, (OggzFunc)oggz_comment_free);
  oggz_vector_delete (stream->comments);
  stream->comments = NULL;

  if (stream->vendor) oggz_free (stream->vendor);
  stream->vendor = NULL;

  return 0;
}

int
oggz_comments_decode (OGGZ * oggz, long serialno,
                      unsigned char * comments, long length)
{
   oggz_stream_t * stream;
   char *c= (char *)comments;
   int i, nb_fields, n;
   size_t len;
   char *end;
   char * name, * value, * nvalue = NULL;
   OggzComment * comment;

   if (length<8)
      return -1;

   end = c+length;
   len=readint(c, 0);

   c+=4;
   if (len>(size_t)(end-c)) return -1;

   stream = oggz_get_stream (oggz, serialno);
   if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

   
   if (len > 0) {
     if ((nvalue = oggz_strdup_len (c, len)) == NULL)
       return OGGZ_ERR_OUT_OF_MEMORY;

     if (_oggz_comment_set_vendor (oggz, serialno, nvalue) == OGGZ_ERR_OUT_OF_MEMORY) {
       oggz_free (nvalue);
       return OGGZ_ERR_OUT_OF_MEMORY;
     }

     oggz_free (nvalue);
   }

#ifdef DEBUG
   fwrite(c, 1, len, stderr); fputc ('\n', stderr);
#endif
   c+=len;

   if (c+4>end) return -1;

   

   nb_fields=readint(c, 0);
   c+=4;
   for (i=0;i<nb_fields;i++) {
      if (c+4>end) return -1;

      len=readint(c, 0);

      c+=4;
      if (len>(size_t)(end-c)) return -1;

      name = c;
      value = oggz_index_len (c, '=', len);
      if (value) {
         *value = '\0';
         value++;

         n = c+len - value;
         if ((nvalue = oggz_strdup_len (value, n)) == NULL)
           return OGGZ_ERR_OUT_OF_MEMORY;

#ifdef DEBUG
         printf ("oggz_comments_decode: %s -> %s (length %d)\n",
         name, nvalue, n);
#endif
         if (_oggz_comment_add_byname (stream, name, nvalue) == NULL) {
           oggz_free (nvalue);
           return OGGZ_ERR_OUT_OF_MEMORY;
	 }

         oggz_free (nvalue);
      } else {
         if ((nvalue = oggz_strdup_len (name, len)) == NULL)
           return OGGZ_ERR_OUT_OF_MEMORY;

         if (_oggz_comment_add_byname (stream, nvalue, NULL) == NULL) {
           oggz_free (nvalue);
           return OGGZ_ERR_OUT_OF_MEMORY;
	 }

         oggz_free (nvalue);
      }

      c+=len;
   }

#ifdef DEBUG
   printf ("oggz_comments_decode: done\n");
#endif

   return 0;
}






static unsigned long
accum_length (unsigned long * accum, unsigned long delta)
{
  
  assert (*accum != 0 || delta != 0);

  
  if (delta > ULONG_MAX - (*accum))
    return 0;

  *accum += delta;

  return *accum;
}

long
oggz_comments_encode (OGGZ * oggz, long serialno,
                      unsigned char * buf, long length)
{
  oggz_stream_t * stream;
  char * c = (char *)buf;
  const OggzComment * comment;
  int nb_fields = 0, vendor_length = 0;
  unsigned long actual_length = 0, remaining = length, field_length;

  
  if (length < 0) return 0;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  
  if (stream->vendor)
    vendor_length = oggz_comment_len (stream->vendor);
  if (accum_length (&actual_length, 4 + vendor_length) == 0)
    return 0;
#ifdef DEBUG
  printf ("oggz_comments_encode: vendor = %s\n", stream->vendor);
#endif

  
  if (accum_length (&actual_length, 4) == 0)
    return 0;


  for (comment = oggz_comment_first (oggz, serialno); comment;
       comment = oggz_comment_next (oggz, serialno, comment)) {
    
    if (accum_length (&actual_length, 4 + oggz_comment_len (comment->name)) == 0)
      return 0;
    if (comment->value) {
      
      if (accum_length (&actual_length, 1 + oggz_comment_len (comment->value)) == 0)
        return 0;
    }

#ifdef DEBUG
    printf ("oggz_comments_encode: %s = %s\n",
	    comment->name, comment->value);
#endif

    nb_fields++;
  }

  
  if (accum_length (&actual_length, 1) == 0)
    return 0;

  

  if (buf == NULL) return actual_length;

  remaining -= 4;
  if (remaining <= 0) return actual_length;
  writeint (c, 0, vendor_length);
  c += 4;

  if (stream->vendor) {
    field_length = oggz_comment_len (stream->vendor);
    memcpy (c, stream->vendor, MIN (field_length, remaining));
    c += field_length; remaining -= field_length;
    if (remaining <= 0) return actual_length;
  }

  remaining -= 4;
  if (remaining <= 0) return actual_length;
  writeint (c, 0, nb_fields);
  c += 4;

  for (comment = oggz_comment_first (oggz, serialno); comment;
       comment = oggz_comment_next (oggz, serialno, comment)) {

    field_length = oggz_comment_len (comment->name);     
    if (comment->value)
      field_length += 1 + oggz_comment_len (comment->value); 

    remaining -= 4;
    if (remaining <= 0) return actual_length;
    writeint (c, 0, field_length);
    c += 4;

    field_length = oggz_comment_len (comment->name);
    memcpy (c, comment->name, MIN (field_length, remaining));
    c += field_length; remaining -= field_length;
    if (remaining <= 0) return actual_length;

    if (comment->value) {
      remaining --;
      if (remaining <= 0) return actual_length;
      *c = '=';
      c++;

      field_length = oggz_comment_len (comment->value);
      memcpy (c, comment->value, MIN (field_length, remaining));
      c += field_length; remaining -= field_length;
      if (remaining <= 0) return actual_length;
    }
  }

  if (remaining <= 0) return actual_length;
  *c = 0x01;

  return actual_length;
}



ogg_packet *
oggz_comment_generate(OGGZ * oggz, long serialno,
		      OggzStreamContent packet_type,
		      int FLAC_final_metadata_block)
{
  ogg_packet *c_packet;

  unsigned char *buffer;
  unsigned const char *preamble;
  long preamble_length, comment_length, buf_size;

  




  const unsigned char preamble_vorbis[7] =
    {0x03, 0x76, 0x6f, 0x72, 0x62, 0x69, 0x73};
  const unsigned char preamble_theora[7] =
    {0x81, 0x74, 0x68, 0x65, 0x6f, 0x72, 0x61};
  const unsigned char preamble_flac[4] =
    {0x04, 0x00, 0x00, 0x00};
  const unsigned char preamble_kate[9] =
    {0x81, 0x6b, 0x61, 0x74, 0x65, 0x00, 0x00, 0x00, 0x00};


  switch(packet_type) {
    case OGGZ_CONTENT_VORBIS:
      preamble_length = sizeof preamble_vorbis;
      preamble = preamble_vorbis;
      break;
    case OGGZ_CONTENT_THEORA:
      preamble_length = sizeof preamble_theora;
      preamble = preamble_theora;
      break;
    case OGGZ_CONTENT_FLAC:
      preamble_length = sizeof preamble_flac;
      preamble = preamble_flac;
      break;
    case OGGZ_CONTENT_KATE:
      preamble_length = sizeof preamble_kate;
      preamble = preamble_kate;
      break;
    case OGGZ_CONTENT_PCM:
    case OGGZ_CONTENT_SPEEX:
      preamble_length = 0;
      preamble = 0;
      
      break;
    default:
      return NULL;
  }

  comment_length = oggz_comments_encode (oggz, serialno, 0, 0);
  if(comment_length <= 0) {
    return NULL;
  }

  buf_size = preamble_length + comment_length;

  if(packet_type == OGGZ_CONTENT_FLAC && comment_length >= 0x00ffffff) {
    return NULL;
  }

  c_packet = oggz_malloc(sizeof *c_packet);
  if(c_packet) {
    memset(c_packet, 0, sizeof *c_packet);
    c_packet->packetno = 1;
    c_packet->packet = oggz_malloc(buf_size);
  }

  if(c_packet && c_packet->packet) {
    buffer = c_packet->packet;
    if(preamble_length) {
      memcpy(buffer, preamble, preamble_length);
      if(packet_type == OGGZ_CONTENT_FLAC) {
	

	
	writeint24be(c_packet->packet, 1, (comment_length-1) );
	if(FLAC_final_metadata_block) 
	  {
	    c_packet->packet[0] |= 0x80;
	  }
      }
      buffer += preamble_length;
    }
    oggz_comments_encode (oggz, serialno, buffer, comment_length);
    c_packet->bytes = buf_size;
    

    if(packet_type != OGGZ_CONTENT_VORBIS)
      {
	c_packet->bytes -= 1;
      }
  } else {
    oggz_free(c_packet);
    c_packet = 0;
  }

  return c_packet;
}



ogg_packet *
oggz_comments_generate(OGGZ * oggz, long serialno,
		      int FLAC_final_metadata_block)
{
  OggzStreamContent packet_type;

  packet_type = oggz_stream_get_content (oggz, serialno);

  return oggz_comment_generate (oggz, serialno, packet_type,
                                FLAC_final_metadata_block);
}

void oggz_packet_destroy(ogg_packet *packet) {
  if(packet) {
    if(packet->packet)
      {
	oggz_free(packet->packet);
      }
    oggz_free(packet);
  }
  return;
}
