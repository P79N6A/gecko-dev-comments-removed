









































#include <limits.h>

#include <cairoint.h>

#define UTF8_COMPUTE(Char, Mask, Len)					      \
  if (Char < 128)							      \
    {									      \
      Len = 1;								      \
      Mask = 0x7f;							      \
    }									      \
  else if ((Char & 0xe0) == 0xc0)					      \
    {									      \
      Len = 2;								      \
      Mask = 0x1f;							      \
    }									      \
  else if ((Char & 0xf0) == 0xe0)					      \
    {									      \
      Len = 3;								      \
      Mask = 0x0f;							      \
    }									      \
  else if ((Char & 0xf8) == 0xf0)					      \
    {									      \
      Len = 4;								      \
      Mask = 0x07;							      \
    }									      \
  else if ((Char & 0xfc) == 0xf8)					      \
    {									      \
      Len = 5;								      \
      Mask = 0x03;							      \
    }									      \
  else if ((Char & 0xfe) == 0xfc)					      \
    {									      \
      Len = 6;								      \
      Mask = 0x01;							      \
    }									      \
  else									      \
    Len = -1;

#define UTF8_LENGTH(Char)              \
  ((Char) < 0x80 ? 1 :                 \
   ((Char) < 0x800 ? 2 :               \
    ((Char) < 0x10000 ? 3 :            \
     ((Char) < 0x200000 ? 4 :          \
      ((Char) < 0x4000000 ? 5 : 6)))))

#define UTF8_GET(Result, Chars, Count, Mask, Len)			      \
  (Result) = (Chars)[0] & (Mask);					      \
  for ((Count) = 1; (Count) < (Len); ++(Count))				      \
    {									      \
      if (((Chars)[(Count)] & 0xc0) != 0x80)				      \
	{								      \
	  (Result) = -1;						      \
	  break;							      \
	}								      \
      (Result) <<= 6;							      \
      (Result) |= ((Chars)[(Count)] & 0x3f);				      \
    }

#define UNICODE_VALID(Char)                   \
    ((Char) < 0x110000 &&                     \
     (((Char) & 0xFFFFF800) != 0xD800) &&     \
     ((Char) < 0xFDD0 || (Char) > 0xFDEF) &&  \
     ((Char) & 0xFFFE) != 0xFFFE)

static const char utf8_skip_data[256] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};

#define UTF8_NEXT_CHAR(p) ((p) + utf8_skip_data[*(unsigned char *)(p)])





static uint32_t
_utf8_get_char (const unsigned char *p)
{
    int i, mask = 0, len;
    uint32_t result;
    unsigned char c = (unsigned char) *p;

    UTF8_COMPUTE (c, mask, len);
    if (len == -1)
	return (uint32_t)-1;
    UTF8_GET (result, p, i, mask, len);

    return result;
}




static uint32_t
_utf8_get_char_extended (const unsigned char *p,
			 long		      max_len)
{
    int i, len;
    uint32_t wc = (unsigned char) *p;

    if (wc < 0x80) {
	return wc;
    } else if (wc < 0xc0) {
	return (uint32_t)-1;
    } else if (wc < 0xe0) {
	len = 2;
	wc &= 0x1f;
    } else if (wc < 0xf0) {
	len = 3;
	wc &= 0x0f;
    } else if (wc < 0xf8) {
	len = 4;
	wc &= 0x07;
    } else if (wc < 0xfc) {
	len = 5;
	wc &= 0x03;
    } else if (wc < 0xfe) {
	len = 6;
	wc &= 0x01;
    } else {
	return (uint32_t)-1;
    }

    if (max_len >= 0 && len > max_len) {
	for (i = 1; i < max_len; i++) {
	    if ((((unsigned char *)p)[i] & 0xc0) != 0x80)
		return (uint32_t)-1;
	}
	return (uint32_t)-2;
    }

    for (i = 1; i < len; ++i) {
	uint32_t ch = ((unsigned char *)p)[i];

	if ((ch & 0xc0) != 0x80) {
	    if (ch)
		return (uint32_t)-1;
	    else
		return (uint32_t)-2;
	}

	wc <<= 6;
	wc |= (ch & 0x3f);
    }

    if (UTF8_LENGTH(wc) != len)
	return (uint32_t)-1;

    return wc;
}





















cairo_status_t
_cairo_utf8_to_ucs4 (const unsigned char *str,
		     int		  len,
		     uint32_t		**result,
		     int		 *items_written)
{
    uint32_t *str32 = NULL;
    int n_chars, i;
    const unsigned char *in;

    in = str;
    n_chars = 0;
    while ((len < 0 || str + len - in > 0) && *in)
    {
	uint32_t wc = _utf8_get_char_extended (in, str + len - in);
	if (wc & 0x80000000 || !UNICODE_VALID (wc))
	    return CAIRO_STATUS_INVALID_STRING;

	n_chars++;
	if (n_chars == INT_MAX)
	    return CAIRO_STATUS_INVALID_STRING;

	in = UTF8_NEXT_CHAR (in);
    }

    str32 = malloc (sizeof (uint32_t) * (n_chars + 1));
    if (!str32)
	return CAIRO_STATUS_NO_MEMORY;

    in = str;
    for (i=0; i < n_chars; i++) {
	str32[i] = _utf8_get_char (in);
	in = UTF8_NEXT_CHAR (in);
    }
    str32[i] = 0;

    *result = str32;
    if (items_written)
	*items_written = n_chars;

    return CAIRO_STATUS_SUCCESS;
}






















cairo_status_t
_cairo_utf8_to_utf16 (const unsigned char *str,
		      int		   len,
		      uint16_t		 **result,
		      int		  *items_written)
{
    uint16_t *str16 = NULL;
    int n16, i;
    const unsigned char *in;

    in = str;
    n16 = 0;
    while ((len < 0 || str + len - in > 0) && *in) {
	uint32_t wc = _utf8_get_char_extended (in, str + len - in);
	if (wc & 0x80000000 || !UNICODE_VALID (wc))
	    return CAIRO_STATUS_INVALID_STRING;

	if (wc < 0x10000)
	    n16 += 1;
	else
	    n16 += 2;

	if (n16 == INT_MAX - 1 || n16 == INT_MAX)
	    return CAIRO_STATUS_INVALID_STRING;

	in = UTF8_NEXT_CHAR (in);
    }

    str16 = malloc (sizeof (uint16_t) * (n16 + 1));
    if (!str16)
	return CAIRO_STATUS_NO_MEMORY;

    in = str;
    for (i = 0; i < n16;) {
	uint32_t wc = _utf8_get_char (in);

	if (wc < 0x10000) {
	    str16[i++] = wc;
	} else {
	    str16[i++] = (wc - 0x10000) / 0x400 + 0xd800;
	    str16[i++] = (wc - 0x10000) % 0x400 + 0xdc00;
	}

	in = UTF8_NEXT_CHAR (in);
    }

    str16[i] = 0;

    *result = str16;
    if (items_written)
	*items_written = n16;

    return CAIRO_STATUS_SUCCESS;
}
