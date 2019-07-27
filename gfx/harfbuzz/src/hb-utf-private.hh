

























#ifndef HB_UTF_PRIVATE_HH
#define HB_UTF_PRIVATE_HH

#include "hb-private.hh"

template <typename T, bool validate=true> struct hb_utf_t;




template <>
struct hb_utf_t<uint8_t, true>
{
  static inline const uint8_t *
  next (const uint8_t *text,
	const uint8_t *end,
	hb_codepoint_t *unicode,
	hb_codepoint_t replacement)
  {
    



    hb_codepoint_t c = *text++;

    if (c > 0x7Fu)
    {
      if (hb_in_range (c, 0xC2u, 0xDFu)) 
      {
	unsigned int t1;
	if (likely (text < end &&
		    (t1 = text[0] - 0x80u) <= 0x3Fu))
	{
	  c = ((c&0x1Fu)<<6) | t1;
	  text++;
	}
	else
	  goto error;
      }
      else if (hb_in_range (c, 0xE0u, 0xEFu)) 
      {
	unsigned int t1, t2;
	if (likely (1 < end - text &&
		    (t1 = text[0] - 0x80u) <= 0x3Fu &&
		    (t2 = text[1] - 0x80u) <= 0x3Fu))
	{
	  c = ((c&0xFu)<<12) | (t1<<6) | t2;
	  if (unlikely (c < 0x0800u || hb_in_range (c, 0xD800u, 0xDFFFu)))
	    goto error;
	  text += 2;
	}
	else
	  goto error;
      }
      else if (hb_in_range (c, 0xF0u, 0xF4u)) 
      {
	unsigned int t1, t2, t3;
	if (likely (2 < end - text &&
		    (t1 = text[0] - 0x80u) <= 0x3Fu &&
		    (t2 = text[1] - 0x80u) <= 0x3Fu &&
		    (t3 = text[2] - 0x80u) <= 0x3Fu))
	{
	  c = ((c&0x7u)<<18) | (t1<<12) | (t2<<6) | t3;
	  if (unlikely (!hb_in_range (c, 0x10000u, 0x10FFFFu)))
	    goto error;
	  text += 3;
	}
	else
	  goto error;
      }
      else
	goto error;
    }

    *unicode = c;
    return text;

  error:
    *unicode = replacement;
    return text;
  }

  static inline const uint8_t *
  prev (const uint8_t *text,
	const uint8_t *start,
	hb_codepoint_t *unicode,
	hb_codepoint_t replacement)
  {
    const uint8_t *end = text--;
    while (start < text && (*text & 0xc0) == 0x80 && end - text < 4)
      text--;

    if (likely (next (text, end, unicode, replacement) == end))
      return text;

    *unicode = replacement;
    return end - 1;
  }

  static inline unsigned int
  strlen (const uint8_t *text)
  {
    return ::strlen ((const char *) text);
  }
};




template <>
struct hb_utf_t<uint16_t, true>
{
  static inline const uint16_t *
  next (const uint16_t *text,
	const uint16_t *end,
	hb_codepoint_t *unicode,
	hb_codepoint_t replacement)
  {
    hb_codepoint_t c = *text++;

    if (likely (!hb_in_range (c, 0xD800u, 0xDFFFu)))
    {
      *unicode = c;
      return text;
    }

    if (likely (hb_in_range (c, 0xD800u, 0xDBFFu)))
    {
      
      hb_codepoint_t l;
      if (text < end && ((l = *text), likely (hb_in_range (l, 0xDC00u, 0xDFFFu))))
      {
	
	*unicode = (c << 10) + l - ((0xD800u << 10) - 0x10000u + 0xDC00u);
	 text++;
	 return text;
      }
    }

    
    *unicode = replacement;
    return text;
  }

  static inline const uint16_t *
  prev (const uint16_t *text,
	const uint16_t *start,
	hb_codepoint_t *unicode,
	hb_codepoint_t replacement)
  {
    const uint16_t *end = text--;
    hb_codepoint_t c = *text;

    if (likely (!hb_in_range (c, 0xD800u, 0xDFFFu)))
    {
      *unicode = c;
      return text;
    }

    if (likely (start < text && hb_in_range (c, 0xDC00u, 0xDFFFu)))
      text--;

    if (likely (next (text, end, unicode, replacement) == end))
      return text;

    *unicode = replacement;
    return end - 1;
  }


  static inline unsigned int
  strlen (const uint16_t *text)
  {
    unsigned int l = 0;
    while (*text++) l++;
    return l;
  }
};




template <bool validate>
struct hb_utf_t<uint32_t, validate>
{
  static inline const uint32_t *
  next (const uint32_t *text,
	const uint32_t *end HB_UNUSED,
	hb_codepoint_t *unicode,
	hb_codepoint_t replacement)
  {
    hb_codepoint_t c = *text++;
    if (validate && unlikely (c > 0x10FFFFu || hb_in_range (c, 0xD800u, 0xDFFFu)))
      goto error;
    *unicode = c;
    return text;

  error:
    *unicode = replacement;
    return text;
  }

  static inline const uint32_t *
  prev (const uint32_t *text,
	const uint32_t *start HB_UNUSED,
	hb_codepoint_t *unicode,
	hb_codepoint_t replacement)
  {
    next (text - 1, text, unicode, replacement);
    return text - 1;
  }

  static inline unsigned int
  strlen (const uint32_t *text)
  {
    unsigned int l = 0;
    while (*text++) l++;
    return l;
  }
};


#endif 
