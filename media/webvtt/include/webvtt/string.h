


























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






WEBVTT_EXPORT void
webvtt_init_string( webvtt_string *result );








WEBVTT_EXPORT webvtt_uint
webvtt_string_is_empty( const webvtt_string *str );









WEBVTT_EXPORT webvtt_status
webvtt_create_string( webvtt_uint32 alloc, webvtt_string *result );







WEBVTT_EXPORT webvtt_status
webvtt_create_string_with_text( webvtt_string *out, const char *init_text,
                                int len );








WEBVTT_EXPORT void
webvtt_ref_string( webvtt_string *str );








WEBVTT_EXPORT void
webvtt_release_string( webvtt_string *str );









WEBVTT_EXPORT webvtt_status
webvtt_string_detach( webvtt_string *str );






WEBVTT_EXPORT void
webvtt_copy_string( webvtt_string *left, const webvtt_string *right );






WEBVTT_EXPORT const char *
webvtt_string_text( const webvtt_string *str );






WEBVTT_EXPORT webvtt_uint32
webvtt_string_length( const webvtt_string *str );






WEBVTT_EXPORT webvtt_uint32
webvtt_string_capacity( const webvtt_string *str );







WEBVTT_EXPORT int
webvtt_string_getline( webvtt_string *str, const char *buffer,
                       webvtt_uint *pos, int len, int *truncate,
                       webvtt_bool finish );






WEBVTT_EXPORT webvtt_status
webvtt_string_putc( webvtt_string *str, char to_append );






WEBVTT_EXPORT webvtt_status
webvtt_string_replace( webvtt_string *str, const char *search, int search_len,
                       const char *replace, int replace_len );






WEBVTT_EXPORT webvtt_status
webvtt_string_replace_all( webvtt_string *str, const char *search,
                           int search_len, const char *replace,
                           int replace_len );







WEBVTT_EXPORT webvtt_bool
webvtt_string_is_equal( const webvtt_string *str, const char *to_compare,
                        int len );








WEBVTT_EXPORT webvtt_status
webvtt_string_append( webvtt_string *str, const char *buffer, int len );







WEBVTT_EXPORT webvtt_status
webvtt_string_append_string( webvtt_string *str, const webvtt_string *other );




struct
webvtt_stringlist_t {
  struct webvtt_refcount_t refs;
  webvtt_uint alloc;
  webvtt_uint length;
  webvtt_string *items;
};






WEBVTT_EXPORT webvtt_status
webvtt_create_stringlist( webvtt_stringlist **result );






WEBVTT_EXPORT void
webvtt_ref_stringlist( webvtt_stringlist *list );






WEBVTT_EXPORT void
webvtt_copy_stringlist( webvtt_stringlist **left, webvtt_stringlist *right );






WEBVTT_EXPORT void
webvtt_release_stringlist( webvtt_stringlist **list );






WEBVTT_EXPORT webvtt_status
webvtt_stringlist_push( webvtt_stringlist *list, webvtt_string *str );






WEBVTT_EXPORT webvtt_bool
webvtt_stringlist_pop( webvtt_stringlist *list, webvtt_string *out );











WEBVTT_EXPORT webvtt_bool
webvtt_next_utf8( const char **begin, const char *end );









WEBVTT_EXPORT webvtt_bool
webvtt_skip_utf8( const char **begin, const char *end, int n_chars );






WEBVTT_EXPORT webvtt_uint16
webvtt_utf8_to_utf16( const char *utf8, const char *end, webvtt_uint16 *high );







WEBVTT_EXPORT int
webvtt_utf8_chcount( const char *utf8, const char *end );








WEBVTT_EXPORT int
webvtt_utf8_length( const char *utf8 );

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif
