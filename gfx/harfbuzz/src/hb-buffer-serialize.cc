

























#include "hb-buffer-private.hh"


static const char *serialize_formats[] = {
  "text",
  "json",
  NULL
};










const char **
hb_buffer_serialize_list_formats (void)
{
  return serialize_formats;
}












hb_buffer_serialize_format_t
hb_buffer_serialize_format_from_string (const char *str, int len)
{
  
  return (hb_buffer_serialize_format_t) (hb_tag_from_string (str, len) & ~0x20202020u);
}











const char *
hb_buffer_serialize_format_to_string (hb_buffer_serialize_format_t format)
{
  switch (format)
  {
    case HB_BUFFER_SERIALIZE_FORMAT_TEXT:	return serialize_formats[0];
    case HB_BUFFER_SERIALIZE_FORMAT_JSON:	return serialize_formats[1];
    default:
    case HB_BUFFER_SERIALIZE_FORMAT_INVALID:	return NULL;
  }
}

static unsigned int
_hb_buffer_serialize_glyphs_json (hb_buffer_t *buffer,
				  unsigned int start,
				  unsigned int end,
				  char *buf,
				  unsigned int buf_size,
				  unsigned int *buf_consumed,
				  hb_font_t *font,
				  hb_buffer_serialize_flags_t flags)
{
  hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buffer, NULL);
  hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (buffer, NULL);

  *buf_consumed = 0;
  for (unsigned int i = start; i < end; i++)
  {
    char b[1024];
    char *p = b;

    

#define APPEND(s) HB_STMT_START { strcpy (p, s); p += strlen (s); } HB_STMT_END

    if (i)
      *p++ = ',';

    *p++ = '{';

    APPEND ("\"g\":");
    if (!(flags & HB_BUFFER_SERIALIZE_FLAG_NO_GLYPH_NAMES))
    {
      char g[128];
      hb_font_glyph_to_string (font, info[i].codepoint, g, sizeof (g));
      *p++ = '"';
      for (char *q = g; *q; q++) {
        if (*q == '"')
	  *p++ = '\\';
	*p++ = *q;
      }
      *p++ = '"';
    }
    else
      p += MAX (0, snprintf (p, ARRAY_LENGTH (b) - (p - b), "%u", info[i].codepoint));

    if (!(flags & HB_BUFFER_SERIALIZE_FLAG_NO_CLUSTERS)) {
      p += MAX (0, snprintf (p, ARRAY_LENGTH (b) - (p - b), ",\"cl\":%u", info[i].cluster));
    }

    if (!(flags & HB_BUFFER_SERIALIZE_FLAG_NO_POSITIONS))
    {
      p += snprintf (p, ARRAY_LENGTH (b) - (p - b), ",\"dx\":%d,\"dy\":%d",
		     pos[i].x_offset, pos[i].y_offset);
      p += snprintf (p, ARRAY_LENGTH (b) - (p - b), ",\"ax\":%d,\"ay\":%d",
		     pos[i].x_advance, pos[i].y_advance);
    }

    *p++ = '}';

    unsigned int l = p - b;
    if (buf_size > l)
    {
      memcpy (buf, b, l);
      buf += l;
      buf_size -= l;
      *buf_consumed += l;
      *buf = '\0';
    } else
      return i - start;
  }

  return end - start;
}

static unsigned int
_hb_buffer_serialize_glyphs_text (hb_buffer_t *buffer,
				  unsigned int start,
				  unsigned int end,
				  char *buf,
				  unsigned int buf_size,
				  unsigned int *buf_consumed,
				  hb_font_t *font,
				  hb_buffer_serialize_flags_t flags)
{
  hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buffer, NULL);
  hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (buffer, NULL);

  *buf_consumed = 0;
  for (unsigned int i = start; i < end; i++)
  {
    char b[1024];
    char *p = b;

    

    if (i)
      *p++ = '|';

    if (!(flags & HB_BUFFER_SERIALIZE_FLAG_NO_GLYPH_NAMES))
    {
      hb_font_glyph_to_string (font, info[i].codepoint, p, 128);
      p += strlen (p);
    }
    else
      p += MAX (0, snprintf (p, ARRAY_LENGTH (b) - (p - b), "%u", info[i].codepoint));

    if (!(flags & HB_BUFFER_SERIALIZE_FLAG_NO_CLUSTERS)) {
      p += MAX (0, snprintf (p, ARRAY_LENGTH (b) - (p - b), "=%u", info[i].cluster));
    }

    if (!(flags & HB_BUFFER_SERIALIZE_FLAG_NO_POSITIONS))
    {
      if (pos[i].x_offset || pos[i].y_offset)
	p += MAX (0, snprintf (p, ARRAY_LENGTH (b) - (p - b), "@%d,%d", pos[i].x_offset, pos[i].y_offset));

      *p++ = '+';
      p += MAX (0, snprintf (p, ARRAY_LENGTH (b) - (p - b), "%d", pos[i].x_advance));
      if (pos[i].y_advance)
	p += MAX (0, snprintf (p, ARRAY_LENGTH (b) - (p - b), ",%d", pos[i].y_advance));
    }

    unsigned int l = p - b;
    if (buf_size > l)
    {
      memcpy (buf, b, l);
      buf += l;
      buf_size -= l;
      *buf_consumed += l;
      *buf = '\0';
    } else
      return i - start;
  }

  return end - start;
}




















unsigned int
hb_buffer_serialize_glyphs (hb_buffer_t *buffer,
			    unsigned int start,
			    unsigned int end,
			    char *buf,
			    unsigned int buf_size,
			    unsigned int *buf_consumed, 
			    hb_font_t *font, 
			    hb_buffer_serialize_format_t format,
			    hb_buffer_serialize_flags_t flags)
{
  assert (start <= end && end <= buffer->len);

  unsigned int sconsumed;
  if (!buf_consumed)
    buf_consumed = &sconsumed;
  *buf_consumed = 0;

  assert ((!buffer->len && buffer->content_type == HB_BUFFER_CONTENT_TYPE_INVALID) ||
	  buffer->content_type == HB_BUFFER_CONTENT_TYPE_GLYPHS);

  if (unlikely (start == end))
    return 0;

  if (!font)
    font = hb_font_get_empty ();

  switch (format)
  {
    case HB_BUFFER_SERIALIZE_FORMAT_TEXT:
      return _hb_buffer_serialize_glyphs_text (buffer, start, end,
					       buf, buf_size, buf_consumed,
					       font, flags);

    case HB_BUFFER_SERIALIZE_FORMAT_JSON:
      return _hb_buffer_serialize_glyphs_json (buffer, start, end,
					       buf, buf_size, buf_consumed,
					       font, flags);

    default:
    case HB_BUFFER_SERIALIZE_FORMAT_INVALID:
      return 0;

  }
}


static hb_bool_t
parse_uint (const char *pp, const char *end, uint32_t *pv)
{
  char buf[32];
  unsigned int len = MIN (ARRAY_LENGTH (buf) - 1, (unsigned int) (end - pp));
  strncpy (buf, pp, len);
  buf[len] = '\0';

  char *p = buf;
  char *pend = p;
  uint32_t v;

  errno = 0;
  v = strtol (p, &pend, 10);
  if (errno || p == pend || pend - p != end - pp)
    return false;

  *pv = v;
  return true;
}

static hb_bool_t
parse_int (const char *pp, const char *end, int32_t *pv)
{
  char buf[32];
  unsigned int len = MIN (ARRAY_LENGTH (buf) - 1, (unsigned int) (end - pp));
  strncpy (buf, pp, len);
  buf[len] = '\0';

  char *p = buf;
  char *pend = p;
  int32_t v;

  errno = 0;
  v = strtol (p, &pend, 10);
  if (errno || p == pend || pend - p != end - pp)
    return false;

  *pv = v;
  return true;
}

#include "hb-buffer-deserialize-json.hh"
#include "hb-buffer-deserialize-text.hh"
















hb_bool_t
hb_buffer_deserialize_glyphs (hb_buffer_t *buffer,
			      const char *buf,
			      int buf_len, 
			      const char **end_ptr, 
			      hb_font_t *font, 
			      hb_buffer_serialize_format_t format)
{
  const char *end;
  if (!end_ptr)
    end_ptr = &end;
  *end_ptr = buf;

  assert ((!buffer->len && buffer->content_type == HB_BUFFER_CONTENT_TYPE_INVALID) ||
	  buffer->content_type == HB_BUFFER_CONTENT_TYPE_GLYPHS);

  if (buf_len == -1)
    buf_len = strlen (buf);

  if (!buf_len)
  {
    *end_ptr = buf;
    return false;
  }

  hb_buffer_set_content_type (buffer, HB_BUFFER_CONTENT_TYPE_GLYPHS);

  if (!font)
    font = hb_font_get_empty ();

  switch (format)
  {
    case HB_BUFFER_SERIALIZE_FORMAT_TEXT:
      return _hb_buffer_deserialize_glyphs_text (buffer,
						 buf, buf_len, end_ptr,
						 font);

    case HB_BUFFER_SERIALIZE_FORMAT_JSON:
      return _hb_buffer_deserialize_glyphs_json (buffer,
						 buf, buf_len, end_ptr,
						 font);

    default:
    case HB_BUFFER_SERIALIZE_FORMAT_INVALID:
      return false;

  }
}
