

























#include "hb-private.h"

#include "hb-language.h"

HB_BEGIN_DECLS


static const char canon_map[256] = {
   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,  '-',  0,   0,
  '0', '1', '2', '3', '4', '5', '6', '7',  '8', '9',  0,   0,   0,   0,   0,   0,
  '-', 'a', 'b', 'c', 'd', 'e', 'f', 'g',  'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w',  'x', 'y', 'z',  0,   0,   0,   0,  '-',
   0,  'a', 'b', 'c', 'd', 'e', 'f', 'g',  'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w',  'x', 'y', 'z',  0,   0,   0,   0,   0
};

static hb_bool_t
lang_equal (const void *v1,
	    const void *v2)
{
  const unsigned char *p1 = v1;
  const unsigned char *p2 = v2;

  while (canon_map[*p1] && canon_map[*p1] == canon_map[*p2])
    {
      p1++, p2++;
    }

  return (canon_map[*p1] == canon_map[*p2]);
}

#if 0
static unsigned int
lang_hash (const void *key)
{
  const unsigned char *p = key;
  unsigned int h = 0;
  while (canon_map[*p])
    {
      h = (h << 5) - h + canon_map[*p];
      p++;
    }

  return h;
}
#endif


hb_language_t
hb_language_from_string (const char *str)
{
  static unsigned int num_langs;
  static unsigned int num_alloced;
  static const char **langs;
  unsigned int i;
  unsigned char *p;

  

  if (!str)
    return NULL;

  for (i = 0; i < num_langs; i++)
    if (lang_equal (str, langs[i]))
      return langs[i];

  if (unlikely (num_langs == num_alloced)) {
    unsigned int new_alloced = 2 * (8 + num_alloced);
    const char **new_langs = realloc (langs, new_alloced * sizeof (langs[0]));
    if (!new_langs)
      return NULL;
    num_alloced = new_alloced;
    langs = new_langs;
  }

  langs[i] = strdup (str);
  for (p = (unsigned char *) langs[i]; *p; p++)
    *p = canon_map[*p];

  num_langs++;

  return (hb_language_t) langs[i];
}

const char *
hb_language_to_string (hb_language_t language)
{
  return (const char *) language;
}


HB_END_DECLS
