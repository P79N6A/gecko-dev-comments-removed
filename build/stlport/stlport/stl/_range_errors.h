













#ifndef _STLP_RANGE_ERRORS_H
#define _STLP_RANGE_ERRORS_H







#if defined (_STLP_CAN_THROW_RANGE_ERRORS) && defined (_STLP_USE_EXCEPTIONS) && \
   !defined (_STLP_DONT_THROW_RANGE_ERRORS)
#  define _STLP_THROW_RANGE_ERRORS
#endif


#if !defined (_STLP_USE_NO_IOSTREAMS) && !defined (_STLP_EXTERN_RANGE_ERRORS)
#  define _STLP_EXTERN_RANGE_ERRORS
#endif

_STLP_BEGIN_NAMESPACE
void _STLP_FUNCTION_THROWS _STLP_DECLSPEC _STLP_CALL __stl_throw_runtime_error(const char* __msg);
void _STLP_FUNCTION_THROWS _STLP_DECLSPEC _STLP_CALL __stl_throw_range_error(const char* __msg);
void _STLP_FUNCTION_THROWS _STLP_DECLSPEC _STLP_CALL __stl_throw_out_of_range(const char* __msg);
void _STLP_FUNCTION_THROWS _STLP_DECLSPEC _STLP_CALL __stl_throw_length_error(const char* __msg);
void _STLP_FUNCTION_THROWS _STLP_DECLSPEC _STLP_CALL __stl_throw_invalid_argument(const char* __msg);
void _STLP_FUNCTION_THROWS _STLP_DECLSPEC _STLP_CALL __stl_throw_overflow_error(const char* __msg);

#if defined (__DMC__) && !defined (_STLP_NO_EXCEPTIONS)
#   pragma noreturn(__stl_throw_runtime_error)
#   pragma noreturn(__stl_throw_range_error)
#   pragma noreturn(__stl_throw_out_of_range)
#   pragma noreturn(__stl_throw_length_error)
#   pragma noreturn(__stl_throw_invalid_argument)
#   pragma noreturn(__stl_throw_overflow_error)
#endif
_STLP_END_NAMESPACE

#if !defined (_STLP_EXTERN_RANGE_ERRORS)
#  include <stl/_range_errors.c>
#endif

#endif 




