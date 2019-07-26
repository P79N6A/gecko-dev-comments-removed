





























#ifndef _STLP_C_LOCALE_IMPL_H
#define _STLP_C_LOCALE_IMPL_H

#include "stlport_prefix.h"

#include <wchar.h> 
#include <stl/c_locale.h>

struct _Locale_name_hint;

#if defined (_GNU_SOURCE) && defined (__GLIBC__) && \
    ((__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 2))
#  define _STLP_USE_GLIBC2_LOCALIZATION
#  include <nl_types.h>
typedef nl_catd nl_catd_type;
#else
typedef int nl_catd_type;
#endif





#define _Locale_MAX_SIMPLE_NAME 256

#ifdef __cplusplus
extern "C" {
#endif




typedef unsigned short int _Locale_mask_t;




void _Locale_init(void);




void _Locale_final(void);









struct _Locale_ctype* _Locale_ctype_create(const char *, struct _Locale_name_hint*, int * );
struct _Locale_codecvt* _Locale_codecvt_create(const char *, struct _Locale_name_hint*, int * );
struct _Locale_numeric* _Locale_numeric_create(const char *, struct _Locale_name_hint*, int * );
struct _Locale_time* _Locale_time_create(const char *, struct _Locale_name_hint*, int * );
struct _Locale_collate* _Locale_collate_create(const char *, struct _Locale_name_hint*, int * );
struct _Locale_monetary* _Locale_monetary_create(const char *, struct _Locale_name_hint*, int * );
struct _Locale_messages* _Locale_messages_create(const char *, struct _Locale_name_hint*, int * );









#define _STLP_LOC_UNDEFINED 0
#define _STLP_LOC_UNSUPPORTED_FACET_CATEGORY 1
#define _STLP_LOC_UNKNOWN_NAME 2
#define _STLP_LOC_NO_PLATFORM_SUPPORT 3
#define _STLP_LOC_NO_MEMORY 4






void _Locale_ctype_destroy(struct _Locale_ctype *);
void _Locale_codecvt_destroy(struct _Locale_codecvt *);
void _Locale_numeric_destroy(struct _Locale_numeric *);
void _Locale_time_destroy(struct _Locale_time *);
void _Locale_collate_destroy(struct _Locale_collate *);
void _Locale_monetary_destroy(struct _Locale_monetary *);
void _Locale_messages_destroy(struct _Locale_messages *);






const char * _Locale_ctype_default(char * __buf);
const char * _Locale_numeric_default(char * __buf);
const char * _Locale_time_default(char * __buf);
const char * _Locale_collate_default(char * __buf);
const char * _Locale_monetary_default(char * __buf);
const char * _Locale_messages_default(char * __buf);







char const* _Locale_ctype_name(const struct _Locale_ctype *, char* __buf);
char const* _Locale_codecvt_name(const struct _Locale_codecvt *, char* __buf);
char const* _Locale_numeric_name(const struct _Locale_numeric *, char* __buf);
char const* _Locale_time_name(const struct _Locale_time *, char* __buf);
char const* _Locale_collate_name(const struct _Locale_collate *, char*  __buf);
char const* _Locale_monetary_name(const struct _Locale_monetary *, char* __buf);
char const* _Locale_messages_name(const struct _Locale_messages *, char* __buf);








char const* _Locale_extract_ctype_name(const char *cname, char *__buf,
                                       struct _Locale_name_hint* __hint, int *__err_code);
char const* _Locale_extract_numeric_name(const char *cname, char *__buf,
                                         struct _Locale_name_hint* __hint, int *__err_code);
char const* _Locale_extract_time_name(const char *cname, char *__buf,
                                      struct _Locale_name_hint* __hint, int *__err_code);
char const* _Locale_extract_collate_name(const char *cname, char *__buf,
                                         struct _Locale_name_hint* __hint, int *__err_code);
char const* _Locale_extract_monetary_name(const char *cname, char *__buf,
                                          struct _Locale_name_hint* __hint, int *__err_code);
char const* _Locale_extract_messages_name(const char *cname, char *__buf,
                                          struct _Locale_name_hint* __hint, int *__err_code);








struct _Locale_name_hint* _Locale_get_ctype_hint(struct _Locale_ctype*);
struct _Locale_name_hint* _Locale_get_numeric_hint(struct _Locale_numeric*);
struct _Locale_name_hint* _Locale_get_time_hint(struct _Locale_time*);
struct _Locale_name_hint* _Locale_get_collate_hint(struct _Locale_collate*);
struct _Locale_name_hint* _Locale_get_monetary_hint(struct _Locale_monetary*);
struct _Locale_name_hint* _Locale_get_messages_hint(struct _Locale_messages*);















const _Locale_mask_t * _Locale_ctype_table(struct _Locale_ctype *);




int _Locale_toupper(struct _Locale_ctype *, int );
int _Locale_tolower(struct _Locale_ctype *, int );


#ifndef _STLP_NO_WCHAR_T



_Locale_mask_t _WLocale_ctype(struct _Locale_ctype *, wint_t, _Locale_mask_t);
wint_t _WLocale_tolower(struct _Locale_ctype *, wint_t);
wint_t _WLocale_toupper(struct _Locale_ctype *, wint_t);









int _WLocale_mb_cur_max(struct _Locale_codecvt *);





int _WLocale_mb_cur_min(struct _Locale_codecvt *);





int _WLocale_is_stateless(struct _Locale_codecvt *);













size_t _WLocale_mbtowc(struct _Locale_codecvt *,
                       wchar_t * ,
                       const char * , size_t ,
                       mbstate_t *);










size_t _WLocale_wctomb(struct _Locale_codecvt *,
                       char *, size_t,
                       const wchar_t,
                       mbstate_t *);









size_t _WLocale_unshift(struct _Locale_codecvt *,
                        mbstate_t *,
                        char *, size_t, char **);
#endif












int _Locale_strcmp(struct _Locale_collate *,
                   const char * , size_t ,
                   const char * , size_t );
#ifndef _STLP_NO_WCHAR_T
int _WLocale_strcmp(struct _Locale_collate *,
                    const wchar_t * , size_t ,
                    const wchar_t * , size_t );
#endif










size_t _Locale_strxfrm(struct _Locale_collate *,
                       char * , size_t ,
                       const char * , size_t );

#ifndef _STLP_NO_WCHAR_T
size_t _WLocale_strxfrm(struct _Locale_collate *,
                        wchar_t * , size_t ,
                        const wchar_t * , size_t );
#endif










char _Locale_decimal_point(struct _Locale_numeric *);
char _Locale_thousands_sep(struct _Locale_numeric *);
const char * _Locale_grouping(struct _Locale_numeric *);

#ifndef _STLP_NO_WCHAR_T
wchar_t _WLocale_decimal_point(struct _Locale_numeric *);
wchar_t _WLocale_thousands_sep(struct _Locale_numeric *);
#endif





const char * _Locale_true(struct _Locale_numeric *);
const char * _Locale_false(struct _Locale_numeric *);

#ifndef _STLP_NO_WCHAR_T
const wchar_t * _WLocale_true(struct _Locale_numeric *, wchar_t* , size_t );
const wchar_t * _WLocale_false(struct _Locale_numeric *, wchar_t* , size_t );
#endif








const char * _Locale_int_curr_symbol(struct _Locale_monetary *);
const char * _Locale_currency_symbol(struct _Locale_monetary *);
char         _Locale_mon_decimal_point(struct _Locale_monetary *);
char         _Locale_mon_thousands_sep(struct _Locale_monetary *);
const char * _Locale_mon_grouping(struct _Locale_monetary *);
const char * _Locale_positive_sign(struct _Locale_monetary *);
const char * _Locale_negative_sign(struct _Locale_monetary *);
char         _Locale_int_frac_digits(struct _Locale_monetary *);
char         _Locale_frac_digits(struct _Locale_monetary *);
int          _Locale_p_cs_precedes(struct _Locale_monetary *);
int          _Locale_p_sep_by_space(struct _Locale_monetary *);
int          _Locale_p_sign_posn(struct _Locale_monetary *);
int          _Locale_n_cs_precedes(struct _Locale_monetary *);
int          _Locale_n_sep_by_space(struct _Locale_monetary *);
int          _Locale_n_sign_posn(struct _Locale_monetary *);

#ifndef _STLP_NO_WCHAR_T
const wchar_t * _WLocale_int_curr_symbol(struct _Locale_monetary *, wchar_t* , size_t );
const wchar_t * _WLocale_currency_symbol(struct _Locale_monetary *, wchar_t* , size_t );
wchar_t         _WLocale_mon_decimal_point(struct _Locale_monetary *);
wchar_t         _WLocale_mon_thousands_sep(struct _Locale_monetary *);
const wchar_t * _WLocale_positive_sign(struct _Locale_monetary *, wchar_t* , size_t );
const wchar_t * _WLocale_negative_sign(struct _Locale_monetary *, wchar_t* , size_t );
#endif








const char * _Locale_full_monthname(struct _Locale_time *, int );
const char * _Locale_abbrev_monthname(struct _Locale_time *, int );

#ifndef _STLP_NO_WCHAR_T
const wchar_t * _WLocale_full_monthname(struct _Locale_time *, int ,
                                        wchar_t* , size_t );
const wchar_t * _WLocale_abbrev_monthname(struct _Locale_time *, int ,
                                          wchar_t* , size_t );
#endif




const char * _Locale_full_dayofweek(struct _Locale_time *, int );
const char * _Locale_abbrev_dayofweek(struct _Locale_time *, int );

#ifndef _STLP_NO_WCHAR_T
const wchar_t * _WLocale_full_dayofweek(struct _Locale_time *, int ,
                                        wchar_t* , size_t );
const wchar_t * _WLocale_abbrev_dayofweek(struct _Locale_time *, int ,
                                          wchar_t* , size_t );
#endif

const char * _Locale_d_t_fmt(struct _Locale_time *);
const char * _Locale_d_fmt(struct _Locale_time *);
const char * _Locale_t_fmt(struct _Locale_time *);
const char * _Locale_long_d_t_fmt(struct _Locale_time*);
const char * _Locale_long_d_fmt(struct _Locale_time*);

const char * _Locale_am_str(struct _Locale_time *);
const char * _Locale_pm_str(struct _Locale_time *);

#ifndef _STLP_NO_WCHAR_T
const wchar_t * _WLocale_am_str(struct _Locale_time *,
                                wchar_t* , size_t );
const wchar_t * _WLocale_pm_str(struct _Locale_time *,
                                wchar_t* , size_t );
#endif









nl_catd_type _Locale_catopen(struct _Locale_messages*, const char*);





void _Locale_catclose(struct _Locale_messages*, nl_catd_type);






const char * _Locale_catgets(struct _Locale_messages *, nl_catd_type,
                             int, int,const char *);

#ifdef __cplusplus
}
#endif

#endif
