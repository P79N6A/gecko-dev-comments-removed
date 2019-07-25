

























#include "hb-private.h"

HB_BEGIN_DECLS


hb_tag_t
hb_tag_from_string (const char *s)
{
  char tag[4];
  unsigned int i;

  for (i = 0; i < 4 && s[i]; i++)
    tag[i] = s[i];
  for (; i < 4; i++)
    tag[i] = ' ';

  return HB_TAG_STR (tag);
}


HB_END_DECLS
