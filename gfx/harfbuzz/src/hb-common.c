

























#include "hb-private.h"

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
