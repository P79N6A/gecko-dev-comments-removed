


























#ifndef __INTERN_STRING_H__
# define __INTERN_STRING_H__
# include <webvtt/string.h>

# define UTF8_LEFT_TO_RIGHT_1   (0xE2)
# define UTF8_LEFT_TO_RIGHT_2   (0x80)
# define UTF8_LEFT_TO_RIGHT_3   (0x8E)
# define UTF8_RIGHT_TO_LEFT_1   (0xE2)
# define UTF8_RIGHT_TO_LEFT_2   (0x80)
# define UTF8_RIGHT_TO_LEFT_3   (0x8F)
# define UTF8_NO_BREAK_SPACE_1  (0xC2)
# define UTF8_NO_BREAK_SPACE_2  (0xA0)





# define UTF_IS_NONCHAR( C ) \
  ( ( C )>=0xFDD0 && \
  ( ( webvtt_uint32 )( C ) <= 0xfdef || ( ( C ) & 0xFFFE)==0xFFFE) && \
    ( webvtt_uint32 )( C ) <= 0x10FFFF )

# define UTF_HIGH_SURROGATE( C ) ( webvtt_uint16 )( ( ( C ) >> 10 ) + 0xD7C0 )
# define UTF_LOW_SURROGATE( C ) ( webvtt_uint16 )( ( ( C ) & 0x3FF ) | 0xDC00 )

# ifndef WEBVTT_MAX_LINE
#   define WEBVTT_MAX_LINE 0x10000
# endif

# ifdef WEBVTT_INLINE
#   define __WEBVTT_STRING_INLINE WEBVTT_INLINE
# else
#   define __WEBVTT_STRING_INLINE
# endif

struct
webvtt_string_data_t {
  struct webvtt_refcount_t refs;
  webvtt_uint32 alloc;
  webvtt_uint32 length;
  webvtt_byte *text;
  webvtt_byte array[1];
};

static __WEBVTT_STRING_INLINE  int
webvtt_isalpha( webvtt_byte ch )
{
  return ( ( ( ch >= 'A' ) && ( ch <= 'Z' ) ) || ( ( ch >= 'a' ) && ( ch <= 'z' ) ) );
}
static __WEBVTT_STRING_INLINE int
webvtt_isdigit( webvtt_byte ch )
{
  return ( ( ch >= '0' ) && ( ch <= '9' ) );
}

static __WEBVTT_STRING_INLINE int
webvtt_isalphanum( webvtt_byte ch )
{
  return ( webvtt_isalpha( ch ) || webvtt_isdigit( ch ) );
}

static __WEBVTT_STRING_INLINE int
webvtt_iswhite( webvtt_byte ch )
{
  return ( ( ch == '\r' ) || ( ch == '\n' ) || ( ch == '\f' )
           || ( ch == '\t' ) || ( ch == ' ' ) ) ;
}

# undef __WEBVTT_STRING_INLINE
#endif
