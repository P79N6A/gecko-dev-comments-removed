

















#ifndef ACQUIRE_RELEASE_H
#define ACQUIRE_RELEASE_H

#include "c_locale.h"

_STLP_BEGIN_NAMESPACE
_STLP_MOVE_TO_PRIV_NAMESPACE

_Locale_ctype* _STLP_CALL __acquire_ctype(const char* &name, char *buf, _Locale_name_hint* hint, int *__err_code);
_Locale_codecvt* _STLP_CALL __acquire_codecvt(const char* &name, char *buf, _Locale_name_hint* hint, int *__err_code);
_Locale_numeric* _STLP_CALL __acquire_numeric(const char* &name, char *buf, _Locale_name_hint* hint, int *__err_code);
_Locale_collate* _STLP_CALL __acquire_collate(const char* &name, char *buf, _Locale_name_hint* hint, int *__err_code);
_Locale_monetary* _STLP_CALL __acquire_monetary(const char* &name, char *buf, _Locale_name_hint* hint, int *__err_code);
_Locale_time* _STLP_CALL __acquire_time(const char* &name, char *buf, _Locale_name_hint*, int *__err_code);
_Locale_messages* _STLP_CALL __acquire_messages(const char* &name, char *buf, _Locale_name_hint* hint, int *__err_code);

void _STLP_CALL __release_ctype(_Locale_ctype* cat);
void _STLP_CALL __release_codecvt(_Locale_codecvt* cat);
void _STLP_CALL __release_numeric(_Locale_numeric* cat);
void _STLP_CALL __release_collate(_Locale_collate* cat);
void _STLP_CALL __release_monetary(_Locale_monetary* cat);
void _STLP_CALL __release_time(_Locale_time* __time);
void _STLP_CALL __release_messages(_Locale_messages* cat);

_STLP_MOVE_TO_STD_NAMESPACE
_STLP_END_NAMESPACE

#endif 
