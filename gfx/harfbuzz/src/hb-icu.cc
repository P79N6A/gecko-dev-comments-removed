




























#include "hb-private.hh"

#include "hb-icu.h"

#include "hb-unicode-private.hh"

#include <unicode/uchar.h>
#include <unicode/unorm.h>
#include <unicode/ustring.h>
#include <unicode/uversion.h>


hb_script_t
hb_icu_script_to_script (UScriptCode script)
{
  if (unlikely (script == USCRIPT_INVALID_CODE))
    return HB_SCRIPT_INVALID;

  return hb_script_from_string (uscript_getShortName (script), -1);
}

UScriptCode
hb_icu_script_from_script (hb_script_t script)
{
  if (unlikely (script == HB_SCRIPT_INVALID))
    return USCRIPT_INVALID_CODE;

  for (unsigned int i = 0; i < USCRIPT_CODE_LIMIT; i++)
    if (unlikely (hb_icu_script_to_script ((UScriptCode) i) == script))
      return (UScriptCode) i;

  return USCRIPT_UNKNOWN;
}


static hb_unicode_combining_class_t
hb_icu_unicode_combining_class (hb_unicode_funcs_t *ufuncs HB_UNUSED,
				hb_codepoint_t      unicode,
				void               *user_data HB_UNUSED)

{
  return (hb_unicode_combining_class_t) u_getCombiningClass (unicode);
}

static unsigned int
hb_icu_unicode_eastasian_width (hb_unicode_funcs_t *ufuncs HB_UNUSED,
				hb_codepoint_t      unicode,
				void               *user_data HB_UNUSED)
{
  switch (u_getIntPropertyValue(unicode, UCHAR_EAST_ASIAN_WIDTH))
  {
  case U_EA_WIDE:
  case U_EA_FULLWIDTH:
    return 2;
  case U_EA_NEUTRAL:
  case U_EA_AMBIGUOUS:
  case U_EA_HALFWIDTH:
  case U_EA_NARROW:
    return 1;
  }
  return 1;
}

static hb_unicode_general_category_t
hb_icu_unicode_general_category (hb_unicode_funcs_t *ufuncs HB_UNUSED,
				 hb_codepoint_t      unicode,
				 void               *user_data HB_UNUSED)
{
  switch (u_getIntPropertyValue(unicode, UCHAR_GENERAL_CATEGORY))
  {
  case U_UNASSIGNED:			return HB_UNICODE_GENERAL_CATEGORY_UNASSIGNED;

  case U_UPPERCASE_LETTER:		return HB_UNICODE_GENERAL_CATEGORY_UPPERCASE_LETTER;
  case U_LOWERCASE_LETTER:		return HB_UNICODE_GENERAL_CATEGORY_LOWERCASE_LETTER;
  case U_TITLECASE_LETTER:		return HB_UNICODE_GENERAL_CATEGORY_TITLECASE_LETTER;
  case U_MODIFIER_LETTER:		return HB_UNICODE_GENERAL_CATEGORY_MODIFIER_LETTER;
  case U_OTHER_LETTER:			return HB_UNICODE_GENERAL_CATEGORY_OTHER_LETTER;

  case U_NON_SPACING_MARK:		return HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK;
  case U_ENCLOSING_MARK:		return HB_UNICODE_GENERAL_CATEGORY_ENCLOSING_MARK;
  case U_COMBINING_SPACING_MARK:	return HB_UNICODE_GENERAL_CATEGORY_SPACING_MARK;

  case U_DECIMAL_DIGIT_NUMBER:		return HB_UNICODE_GENERAL_CATEGORY_DECIMAL_NUMBER;
  case U_LETTER_NUMBER:			return HB_UNICODE_GENERAL_CATEGORY_LETTER_NUMBER;
  case U_OTHER_NUMBER:			return HB_UNICODE_GENERAL_CATEGORY_OTHER_NUMBER;

  case U_SPACE_SEPARATOR:		return HB_UNICODE_GENERAL_CATEGORY_SPACE_SEPARATOR;
  case U_LINE_SEPARATOR:		return HB_UNICODE_GENERAL_CATEGORY_LINE_SEPARATOR;
  case U_PARAGRAPH_SEPARATOR:		return HB_UNICODE_GENERAL_CATEGORY_PARAGRAPH_SEPARATOR;

  case U_CONTROL_CHAR:			return HB_UNICODE_GENERAL_CATEGORY_CONTROL;
  case U_FORMAT_CHAR:			return HB_UNICODE_GENERAL_CATEGORY_FORMAT;
  case U_PRIVATE_USE_CHAR:		return HB_UNICODE_GENERAL_CATEGORY_PRIVATE_USE;
  case U_SURROGATE:			return HB_UNICODE_GENERAL_CATEGORY_SURROGATE;


  case U_DASH_PUNCTUATION:		return HB_UNICODE_GENERAL_CATEGORY_DASH_PUNCTUATION;
  case U_START_PUNCTUATION:		return HB_UNICODE_GENERAL_CATEGORY_OPEN_PUNCTUATION;
  case U_END_PUNCTUATION:		return HB_UNICODE_GENERAL_CATEGORY_CLOSE_PUNCTUATION;
  case U_CONNECTOR_PUNCTUATION:		return HB_UNICODE_GENERAL_CATEGORY_CONNECT_PUNCTUATION;
  case U_OTHER_PUNCTUATION:		return HB_UNICODE_GENERAL_CATEGORY_OTHER_PUNCTUATION;

  case U_MATH_SYMBOL:			return HB_UNICODE_GENERAL_CATEGORY_MATH_SYMBOL;
  case U_CURRENCY_SYMBOL:		return HB_UNICODE_GENERAL_CATEGORY_CURRENCY_SYMBOL;
  case U_MODIFIER_SYMBOL:		return HB_UNICODE_GENERAL_CATEGORY_MODIFIER_SYMBOL;
  case U_OTHER_SYMBOL:			return HB_UNICODE_GENERAL_CATEGORY_OTHER_SYMBOL;

  case U_INITIAL_PUNCTUATION:		return HB_UNICODE_GENERAL_CATEGORY_INITIAL_PUNCTUATION;
  case U_FINAL_PUNCTUATION:		return HB_UNICODE_GENERAL_CATEGORY_FINAL_PUNCTUATION;
  }

  return HB_UNICODE_GENERAL_CATEGORY_UNASSIGNED;
}

static hb_codepoint_t
hb_icu_unicode_mirroring (hb_unicode_funcs_t *ufuncs HB_UNUSED,
			  hb_codepoint_t      unicode,
			  void               *user_data HB_UNUSED)
{
  return u_charMirror(unicode);
}

static hb_script_t
hb_icu_unicode_script (hb_unicode_funcs_t *ufuncs HB_UNUSED,
		       hb_codepoint_t      unicode,
		       void               *user_data HB_UNUSED)
{
  UErrorCode status = U_ZERO_ERROR;
  UScriptCode scriptCode = uscript_getScript(unicode, &status);

  if (unlikely (U_FAILURE (status)))
    return HB_SCRIPT_UNKNOWN;

  return hb_icu_script_to_script (scriptCode);
}

#if U_ICU_VERSION_MAJOR_NUM >= 49
static const UNormalizer2 *normalizer;
#endif

static hb_bool_t
hb_icu_unicode_compose (hb_unicode_funcs_t *ufuncs HB_UNUSED,
			hb_codepoint_t      a,
			hb_codepoint_t      b,
			hb_codepoint_t     *ab,
			void               *user_data HB_UNUSED)
{
#if U_ICU_VERSION_MAJOR_NUM >= 49
  {
    UChar32 ret = unorm2_composePair (normalizer, a, b);
    if (ret < 0) return false;
    *ab = ret;
    return true;
  }
#endif

  


  UChar utf16[4], normalized[5];
  unsigned int len;
  hb_bool_t ret, err;
  UErrorCode icu_err;

  len = 0;
  err = false;
  U16_APPEND (utf16, len, ARRAY_LENGTH (utf16), a, err);
  if (err) return false;
  U16_APPEND (utf16, len, ARRAY_LENGTH (utf16), b, err);
  if (err) return false;

  icu_err = U_ZERO_ERROR;
  len = unorm_normalize (utf16, len, UNORM_NFC, 0, normalized, ARRAY_LENGTH (normalized), &icu_err);
  if (U_FAILURE (icu_err))
    return false;
  if (u_countChar32 (normalized, len) == 1) {
    U16_GET_UNSAFE (normalized, 0, *ab);
    ret = true;
  } else {
    ret = false;
  }

  return ret;
}

static hb_bool_t
hb_icu_unicode_decompose (hb_unicode_funcs_t *ufuncs HB_UNUSED,
			  hb_codepoint_t      ab,
			  hb_codepoint_t     *a,
			  hb_codepoint_t     *b,
			  void               *user_data HB_UNUSED)
{
#if U_ICU_VERSION_MAJOR_NUM >= 49
  {
    UChar decomposed[4];
    int len;
    UErrorCode icu_err = U_ZERO_ERROR;
    len = unorm2_getRawDecomposition (normalizer, ab, decomposed,
				      ARRAY_LENGTH (decomposed), &icu_err);
    if (U_FAILURE (icu_err) || len < 0) return false;

    len = u_countChar32 (decomposed, len);
    if (len == 1) {
      U16_GET_UNSAFE (decomposed, 0, *a);
      *b = 0;
      return *a != ab;
    } else if (len == 2) {
      len =0;
      U16_NEXT_UNSAFE (decomposed, len, *a);
      U16_NEXT_UNSAFE (decomposed, len, *b);
    }
    return true;
  }
#endif

  


  UChar utf16[2], normalized[2 * HB_UNICODE_MAX_DECOMPOSITION_LEN + 1];
  unsigned int len;
  hb_bool_t ret, err;
  UErrorCode icu_err;

  

  

  len = 0;
  err = false;
  U16_APPEND (utf16, len, ARRAY_LENGTH (utf16), ab, err);
  if (err) return false;

  icu_err = U_ZERO_ERROR;
  len = unorm_normalize (utf16, len, UNORM_NFD, 0, normalized, ARRAY_LENGTH (normalized), &icu_err);
  if (U_FAILURE (icu_err))
    return false;

  len = u_countChar32 (normalized, len);

  if (len == 1) {
    U16_GET_UNSAFE (normalized, 0, *a);
    *b = 0;
    ret = *a != ab;
  } else if (len == 2) {
    len =0;
    U16_NEXT_UNSAFE (normalized, len, *a);
    U16_NEXT_UNSAFE (normalized, len, *b);

    


    UChar recomposed[20];
    icu_err = U_ZERO_ERROR;
    unorm_normalize (normalized, len, UNORM_NFC, 0, recomposed, ARRAY_LENGTH (recomposed), &icu_err);
    if (U_FAILURE (icu_err))
      return false;
    hb_codepoint_t c;
    U16_GET_UNSAFE (recomposed, 0, c);
    if (c != *a && c != ab) {
      *a = c;
      *b = 0;
    }
    ret = true;
  } else {
    

    U16_PREV_UNSAFE (normalized, len, *b); 
    UChar recomposed[18 * 2];
    icu_err = U_ZERO_ERROR;
    len = unorm_normalize (normalized, len, UNORM_NFC, 0, recomposed, ARRAY_LENGTH (recomposed), &icu_err);
    if (U_FAILURE (icu_err))
      return false;
    
    if (unlikely (u_countChar32 (recomposed, len) != 1))
      return false;
    U16_GET_UNSAFE (recomposed, 0, *a);
    ret = true;
  }

  return ret;
}

static unsigned int
hb_icu_unicode_decompose_compatibility (hb_unicode_funcs_t *ufuncs HB_UNUSED,
					hb_codepoint_t      u,
					hb_codepoint_t     *decomposed,
					void               *user_data HB_UNUSED)
{
  UChar utf16[2], normalized[2 * HB_UNICODE_MAX_DECOMPOSITION_LEN + 1];
  unsigned int len;
  int32_t utf32_len;
  hb_bool_t err;
  UErrorCode icu_err;

  
  len = 0;
  err = false;
  U16_APPEND (utf16, len, ARRAY_LENGTH (utf16), u, err);
  if (err)
    return 0;

  
  icu_err = U_ZERO_ERROR;
  len = unorm_normalize (utf16, len, UNORM_NFKD, 0, normalized, ARRAY_LENGTH (normalized), &icu_err);
  if (icu_err)
    return 0;

  
  icu_err = U_ZERO_ERROR;
  u_strToUTF32 ((UChar32*) decomposed, HB_UNICODE_MAX_DECOMPOSITION_LEN, &utf32_len, normalized, len, &icu_err);
  if (icu_err)
    return 0;

  return utf32_len;
}


hb_unicode_funcs_t *
hb_icu_get_unicode_funcs (void)
{
  static const hb_unicode_funcs_t _hb_icu_unicode_funcs = {
    HB_OBJECT_HEADER_STATIC,

    NULL, 
    true, 
    {
#define HB_UNICODE_FUNC_IMPLEMENT(name) hb_icu_unicode_##name,
      HB_UNICODE_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_UNICODE_FUNC_IMPLEMENT
    }
  };

#if U_ICU_VERSION_MAJOR_NUM >= 49
  if (!hb_atomic_ptr_get (&normalizer)) {
    UErrorCode icu_err = U_ZERO_ERROR;
    
    hb_atomic_ptr_cmpexch (&normalizer, NULL, unorm2_getNFCInstance (&icu_err));
  }
#endif
  return const_cast<hb_unicode_funcs_t *> (&_hb_icu_unicode_funcs);
}


