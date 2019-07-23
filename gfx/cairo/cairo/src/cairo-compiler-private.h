




































#ifndef CAIRO_COMPILER_PRIVATE_H
#define CAIRO_COMPILER_PRIVATE_H

#include "cairo.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#if __GNUC__ >= 3 && defined(__ELF__) && !defined(__sun)
# define slim_hidden_proto(name)		slim_hidden_proto1(name, slim_hidden_int_name(name)) cairo_private
# define slim_hidden_proto_no_warn(name)	slim_hidden_proto1(name, slim_hidden_int_name(name)) cairo_private_no_warn
# define slim_hidden_def(name)			slim_hidden_def1(name, slim_hidden_int_name(name))
# define slim_hidden_int_name(name) INT_##name
# define slim_hidden_proto1(name, internal)				\
  extern __typeof (name) name						\
	__asm__ (slim_hidden_asmname (internal))
# define slim_hidden_def1(name, internal)				\
  extern __typeof (name) EXT_##name __asm__(slim_hidden_asmname(name))	\
	__attribute__((__alias__(slim_hidden_asmname(internal))))
# define slim_hidden_ulp		slim_hidden_ulp1(__USER_LABEL_PREFIX__)
# define slim_hidden_ulp1(x)		slim_hidden_ulp2(x)
# define slim_hidden_ulp2(x)		#x
# define slim_hidden_asmname(name)	slim_hidden_asmname1(name)
# define slim_hidden_asmname1(name)	slim_hidden_ulp #name
#else
# define slim_hidden_proto(name)		int _cairo_dummy_prototype(void)
# define slim_hidden_proto_no_warn(name)	int _cairo_dummy_prototype(void)
# define slim_hidden_def(name)			int _cairo_dummy_prototype(void)
#endif

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define CAIRO_PRINTF_FORMAT(fmt_index, va_index) \
	__attribute__((__format__(__printf__, fmt_index, va_index)))
#else
#define CAIRO_PRINTF_FORMAT(fmt_index, va_index)
#endif


#define CAIRO_HAS_HIDDEN_SYMBOLS 1
#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)) && defined(__ELF__) && !defined(__sun)
#define cairo_private_no_warn	__attribute__((__visibility__("hidden")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#define cairo_private_no_warn	__hidden
#else 
#define cairo_private_no_warn
#undef CAIRO_HAS_HIDDEN_SYMBOLS
#endif

#ifndef WARN_UNUSED_RESULT
#define WARN_UNUSED_RESULT
#endif

#define cairo_warn	    WARN_UNUSED_RESULT
#define cairo_private	    cairo_private_no_warn cairo_warn












#if __GNUC__ >= 2 && defined(__ELF__)
# define CAIRO_FUNCTION_ALIAS(old, new)		\
	extern __typeof (new) old		\
	__asm__ ("" #old)			\
	__attribute__((__alias__("" #new)))
#else
# define CAIRO_FUNCTION_ALIAS(old, new)
#endif





















#if __GNUC__ >= 3
#define cairo_pure __attribute__((pure))
#define cairo_const __attribute__((const))
#else
#define cairo_pure
#define cairo_const
#endif

#ifndef __GNUC__
#undef __attribute__
#define __attribute__(x)
#endif

#if (defined(__WIN32__) && !defined(__WINE__)) || defined(_MSC_VER)
#define snprintf _snprintf
#define popen _popen
#endif

#ifdef _MSC_VER
#undef inline
#define inline __inline
#endif

#ifdef __STRICT_ANSI__
#undef inline
#define inline __inline__
#endif

#endif
