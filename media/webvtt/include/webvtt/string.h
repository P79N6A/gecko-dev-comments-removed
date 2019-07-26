


























#ifndef __WEBVTT_STRING_H__
# define __WEBVTT_STRING_H__
# include "util.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif




typedef struct webvtt_string_t webvtt_string;
typedef struct webvtt_string_data_t webvtt_string_data;
typedef struct webvtt_stringlist_t webvtt_stringlist;
struct webvtt_string_data_t;

struct
webvtt_string_t {
  webvtt_string_data *d;
};






WEBVTT_EXPORT void webvtt_init_string( webvtt_string *result );







WEBVTT_EXPORT webvtt_uint webvtt_string_is_empty( const webvtt_string *str );









WEBVTT_EXPORT webvtt_status webvtt_create_string( webvtt_uint32 alloc, webvtt_string *result );







WEBVTT_EXPORT webvtt_status webvtt_create_string_with_text( webvtt_string *result, const webvtt_byte *init_text, int len );








WEBVTT_EXPORT void webvtt_ref_string( webvtt_string *str );








WEBVTT_EXPORT void webvtt_release_string( webvtt_string *str );









WEBVTT_EXPORT webvtt_status webvtt_string_detach( webvtt_string *str );






WEBVTT_EXPORT void webvtt_copy_string( webvtt_string *left, const webvtt_string *right );






WEBVTT_EXPORT const webvtt_byte *webvtt_string_text( const webvtt_string *str );






WEBVTT_EXPORT const webvtt_uint32 webvtt_string_length( const webvtt_string *str );






WEBVTT_EXPORT const webvtt_uint32 webvtt_string_capacity( const webvtt_string *str );







WEBVTT_EXPORT int webvtt_string_getline( webvtt_string *str, const webvtt_byte *buffer,
    webvtt_uint *pos, webvtt_uint len, int *truncate, webvtt_bool finish, webvtt_bool retain_new_line );






WEBVTT_EXPORT webvtt_status webvtt_string_putc( webvtt_string *str, webvtt_byte to_append );








WEBVTT_EXPORT webvtt_bool webvtt_string_is_equal( webvtt_string *str, webvtt_byte *to_compare, webvtt_uint len );








WEBVTT_EXPORT webvtt_status webvtt_string_append( webvtt_string *str, const webvtt_byte *buffer, int len );






WEBVTT_EXPORT webvtt_status webvtt_string_append_string( webvtt_string *str, const webvtt_string *other );




struct
webvtt_stringlist_t {
  struct webvtt_refcount_t refs;
  webvtt_uint alloc;
  webvtt_uint length;
  webvtt_string *items;
};






WEBVTT_EXPORT webvtt_status webvtt_create_stringlist( webvtt_stringlist **result );






WEBVTT_EXPORT void webvtt_ref_stringlist( webvtt_stringlist *list );






WEBVTT_EXPORT void webvtt_copy_stringlist( webvtt_stringlist **left, webvtt_stringlist *right );






WEBVTT_EXPORT void webvtt_release_stringlist( webvtt_stringlist **list );






WEBVTT_EXPORT webvtt_status webvtt_stringlist_push( webvtt_stringlist *list, webvtt_string *str );











WEBVTT_EXPORT webvtt_bool webvtt_next_utf8( const webvtt_byte **begin,
  const webvtt_byte *end );









WEBVTT_EXPORT webvtt_bool webvtt_skip_utf8( const webvtt_byte **begin,
  const webvtt_byte *end, int n_chars );






WEBVTT_EXPORT webvtt_uint16 webvtt_utf8_to_utf16( const webvtt_byte *utf8,
  const webvtt_byte *end, webvtt_uint16 *high_surrogate );







WEBVTT_EXPORT int webvtt_utf8_chcount( const webvtt_byte *utf8,
  const webvtt_byte *end );








WEBVTT_EXPORT int webvtt_utf8_length( const webvtt_byte *utf8 );

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif
