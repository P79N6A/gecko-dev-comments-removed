



























#include "hb-private.hh"

#include "hb-version.h"

#include "hb-mutex-private.hh"
#include "hb-object-private.hh"

#include <locale.h>




hb_options_union_t _hb_options;

void
_hb_options_init (void)
{
  hb_options_union_t u;
  u.i = 0;
  u.opts.initialized = 1;

  char *c = getenv ("HB_OPTIONS");
  u.opts.uniscribe_bug_compatible = c && strstr (c, "uniscribe-bug-compatible");

  
  _hb_options = u;
}




hb_tag_t
hb_tag_from_string (const char *s, int len)
{
  char tag[4];
  unsigned int i;

  if (!s || !len || !*s)
    return HB_TAG_NONE;

  if (len < 0 || len > 4)
    len = 4;
  for (i = 0; i < (unsigned) len && s[i]; i++)
    tag[i] = s[i];
  for (; i < 4; i++)
    tag[i] = ' ';

  return HB_TAG_CHAR4 (tag);
}

void
hb_tag_to_string (hb_tag_t tag, char *buf)
{
  buf[0] = (char) (uint8_t) (tag >> 24);
  buf[1] = (char) (uint8_t) (tag >> 16);
  buf[2] = (char) (uint8_t) (tag >>  8);
  buf[3] = (char) (uint8_t) (tag >>  0);
}




const char direction_strings[][4] = {
  "ltr",
  "rtl",
  "ttb",
  "btt"
};

hb_direction_t
hb_direction_from_string (const char *str, int len)
{
  if (unlikely (!str || !len || !*str))
    return HB_DIRECTION_INVALID;

  


  char c = TOLOWER (str[0]);
  for (unsigned int i = 0; i < ARRAY_LENGTH (direction_strings); i++)
    if (c == direction_strings[i][0])
      return (hb_direction_t) (HB_DIRECTION_LTR + i);

  return HB_DIRECTION_INVALID;
}

const char *
hb_direction_to_string (hb_direction_t direction)
{
  if (likely ((unsigned int) (direction - HB_DIRECTION_LTR)
	      < ARRAY_LENGTH (direction_strings)))
    return direction_strings[direction - HB_DIRECTION_LTR];

  return "invalid";
}




struct hb_language_impl_t {
  const char s[1];
};

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
lang_equal (hb_language_t  v1,
	    const void    *v2)
{
  const unsigned char *p1 = (const unsigned char *) v1;
  const unsigned char *p2 = (const unsigned char *) v2;

  while (*p1 && *p1 == canon_map[*p2])
    p1++, p2++;

  return *p1 == canon_map[*p2];
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


struct hb_language_item_t {

  struct hb_language_item_t *next;
  hb_language_t lang;

  inline bool operator == (const char *s) const {
    return lang_equal (lang, s);
  }

  inline hb_language_item_t & operator = (const char *s) {
    lang = (hb_language_t) strdup (s);
    for (unsigned char *p = (unsigned char *) lang; *p; p++)
      *p = canon_map[*p];

    return *this;
  }

  void finish (void) { free (lang); }
};




static hb_language_item_t *langs;

static inline
void free_langs (void)
{
  while (langs) {
    hb_language_item_t *next = langs->next;
    langs->finish ();
    free (langs);
    langs = next;
  }
}

static hb_language_item_t *
lang_find_or_insert (const char *key)
{
retry:
  hb_language_item_t *first_lang = (hb_language_item_t *) hb_atomic_ptr_get (&langs);

  for (hb_language_item_t *lang = first_lang; lang; lang = lang->next)
    if (*lang == key)
      return lang;

  
  hb_language_item_t *lang = (hb_language_item_t *) calloc (1, sizeof (hb_language_item_t));
  if (unlikely (!lang))
    return NULL;
  lang->next = first_lang;
  *lang = key;

  if (!hb_atomic_ptr_cmpexch (&langs, first_lang, lang)) {
    free (lang);
    goto retry;
  }

#ifdef HAVE_ATEXIT
  if (!first_lang)
    atexit (free_langs); 
#endif

  return lang;
}


hb_language_t
hb_language_from_string (const char *str, int len)
{
  if (!str || !len || !*str)
    return HB_LANGUAGE_INVALID;

  char strbuf[32];
  if (len >= 0) {
    len = MIN (len, (int) sizeof (strbuf) - 1);
    str = (char *) memcpy (strbuf, str, len);
    strbuf[len] = '\0';
  }

  hb_language_item_t *item = lang_find_or_insert (str);

  return likely (item) ? item->lang : HB_LANGUAGE_INVALID;
}

const char *
hb_language_to_string (hb_language_t language)
{
  
  return language->s;
}

hb_language_t
hb_language_get_default (void)
{
  static hb_language_t default_language = HB_LANGUAGE_INVALID;

  hb_language_t language = (hb_language_t) hb_atomic_ptr_get (&default_language);
  if (unlikely (language == HB_LANGUAGE_INVALID)) {
    language = hb_language_from_string (setlocale (LC_CTYPE, NULL), -1);
    hb_atomic_ptr_cmpexch (&default_language, HB_LANGUAGE_INVALID, language);
  }

  return default_language;
}




hb_script_t
hb_script_from_iso15924_tag (hb_tag_t tag)
{
  if (unlikely (tag == HB_TAG_NONE))
    return HB_SCRIPT_INVALID;

  
  tag = (tag & 0xDFDFDFDF) | 0x00202020;

  switch (tag) {

    


    case HB_TAG('Q','a','a','i'): return HB_SCRIPT_INHERITED;
    case HB_TAG('Q','a','a','c'): return HB_SCRIPT_COPTIC;

    
    case HB_TAG('C','y','r','s'): return HB_SCRIPT_CYRILLIC;
    case HB_TAG('L','a','t','f'): return HB_SCRIPT_LATIN;
    case HB_TAG('L','a','t','g'): return HB_SCRIPT_LATIN;
    case HB_TAG('S','y','r','e'): return HB_SCRIPT_SYRIAC;
    case HB_TAG('S','y','r','j'): return HB_SCRIPT_SYRIAC;
    case HB_TAG('S','y','r','n'): return HB_SCRIPT_SYRIAC;
  }

  
  if (((uint32_t) tag & 0xE0E0E0E0) == 0x40606060)
    return (hb_script_t) tag;

  
  return HB_SCRIPT_UNKNOWN;
}

hb_script_t
hb_script_from_string (const char *s, int len)
{
  return hb_script_from_iso15924_tag (hb_tag_from_string (s, len));
}

hb_tag_t
hb_script_to_iso15924_tag (hb_script_t script)
{
  return (hb_tag_t) script;
}

hb_direction_t
hb_script_get_horizontal_direction (hb_script_t script)
{
  
  switch ((hb_tag_t) script)
  {
    
    case HB_SCRIPT_ARABIC:
    case HB_SCRIPT_HEBREW:

    
    case HB_SCRIPT_SYRIAC:
    case HB_SCRIPT_THAANA:

    
    case HB_SCRIPT_CYPRIOT:

    
    case HB_SCRIPT_KHAROSHTHI:

    
    case HB_SCRIPT_PHOENICIAN:
    case HB_SCRIPT_NKO:

    
    case HB_SCRIPT_LYDIAN:

    
    case HB_SCRIPT_AVESTAN:
    case HB_SCRIPT_IMPERIAL_ARAMAIC:
    case HB_SCRIPT_INSCRIPTIONAL_PAHLAVI:
    case HB_SCRIPT_INSCRIPTIONAL_PARTHIAN:
    case HB_SCRIPT_OLD_SOUTH_ARABIAN:
    case HB_SCRIPT_OLD_TURKIC:
    case HB_SCRIPT_SAMARITAN:

    
    case HB_SCRIPT_MANDAIC:

    
    case HB_SCRIPT_MEROITIC_CURSIVE:
    case HB_SCRIPT_MEROITIC_HIEROGLYPHS:

      return HB_DIRECTION_RTL;
  }

  return HB_DIRECTION_LTR;
}




bool
hb_user_data_array_t::set (hb_user_data_key_t *key,
			   void *              data,
			   hb_destroy_func_t   destroy,
			   hb_bool_t           replace)
{
  if (!key)
    return false;

  if (replace) {
    if (!data && !destroy) {
      items.remove (key, lock);
      return true;
    }
  }
  hb_user_data_item_t item = {key, data, destroy};
  bool ret = !!items.replace_or_insert (item, lock, replace);

  return ret;
}

void *
hb_user_data_array_t::get (hb_user_data_key_t *key)
{
  hb_user_data_item_t item = {NULL };

  return items.find (key, &item, lock) ? item.data : NULL;
}




void
hb_version (unsigned int *major,
	    unsigned int *minor,
	    unsigned int *micro)
{
  *major = HB_VERSION_MAJOR;
  *minor = HB_VERSION_MINOR;
  *micro = HB_VERSION_MICRO;
}

const char *
hb_version_string (void)
{
  return HB_VERSION_STRING;
}

hb_bool_t
hb_version_check (unsigned int major,
		  unsigned int minor,
		  unsigned int micro)
{
  return HB_VERSION_CHECK (major, minor, micro);
}
