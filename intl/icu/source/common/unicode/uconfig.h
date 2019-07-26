













#ifndef __UCONFIG_H__
#define __UCONFIG_H__



































#if defined(UCONFIG_USE_LOCAL)
#include "uconfig_local.h"
#endif








#ifdef U_DEBUG
    
#elif defined(_DEBUG)
    




#   define U_DEBUG 1
# else
#   define U_DEBUG 0
#endif





#ifndef UCLN_NO_AUTO_CLEANUP
#define UCLN_NO_AUTO_CLEANUP 1
#endif






#ifndef U_DISABLE_RENAMING
#define U_DISABLE_RENAMING 0
#endif









#ifdef U_NO_DEFAULT_INCLUDE_UTF_HEADERS
    
#elif defined(U_COMBINED_IMPLEMENTATION) || defined(U_COMMON_IMPLEMENTATION) || defined(U_I18N_IMPLEMENTATION) || \
      defined(U_IO_IMPLEMENTATION) || defined(U_LAYOUT_IMPLEMENTATION) || defined(U_LAYOUTEX_IMPLEMENTATION) || \
      defined(U_TOOLUTIL_IMPLEMENTATION)
#   define U_NO_DEFAULT_INCLUDE_UTF_HEADERS 1
#else
#   define U_NO_DEFAULT_INCLUDE_UTF_HEADERS 0
#endif














#ifndef U_OVERRIDE_CXX_ALLOCATION
#define U_OVERRIDE_CXX_ALLOCATION 1
#endif






#ifndef U_ENABLE_TRACING
#define U_ENABLE_TRACING 0
#endif






#ifndef U_ENABLE_DYLOAD
#define U_ENABLE_DYLOAD 1
#endif






#ifndef U_CHECK_DYLOAD
#define U_CHECK_DYLOAD 1
#endif







#ifndef U_DEFAULT_SHOW_DRAFT
#define U_DEFAULT_SHOW_DRAFT 1
#endif










#ifdef U_HAVE_LIB_SUFFIX
    
#elif defined(U_LIB_SUFFIX_C_NAME)
#   define U_HAVE_LIB_SUFFIX 1
#endif






#ifdef U_LIB_SUFFIX_C_NAME_STRING
    
#elif defined(U_LIB_SUFFIX_C_NAME)
#   define U_LIB_SUFFIX_C_NAME_STRING #U_LIB_SUFFIX_C_NAME
#else
#   define U_LIB_SUFFIX_C_NAME_STRING ""
#endif














#ifndef UCONFIG_ONLY_COLLATION
#   define UCONFIG_ONLY_COLLATION 0
#endif

#if UCONFIG_ONLY_COLLATION
    
#   define UCONFIG_NO_BREAK_ITERATION 1
#   define UCONFIG_NO_IDNA 1

    
#   if UCONFIG_NO_COLLATION
#       error Contradictory collation switches in uconfig.h.
#   endif
#   define UCONFIG_NO_FORMATTING 1
#   define UCONFIG_NO_TRANSLITERATION 1
#   define UCONFIG_NO_REGULAR_EXPRESSIONS 1
#endif





















#ifndef UCONFIG_NO_FILE_IO
#   define UCONFIG_NO_FILE_IO 0
#endif












#ifndef UCONFIG_NO_CONVERSION
#   define UCONFIG_NO_CONVERSION 0
#endif

#if UCONFIG_NO_CONVERSION
#   define UCONFIG_NO_LEGACY_CONVERSION 1
#endif













#ifndef UCONFIG_NO_LEGACY_CONVERSION
#   define UCONFIG_NO_LEGACY_CONVERSION 0
#endif









#ifndef UCONFIG_NO_NORMALIZATION
#   define UCONFIG_NO_NORMALIZATION 0
#elif UCONFIG_NO_NORMALIZATION
    
    
#   define UCONFIG_NO_BREAK_ITERATION 1
    
#   define UCONFIG_NO_IDNA 1

    
#   if UCONFIG_ONLY_COLLATION
#       error Contradictory collation switches in uconfig.h.
#   endif
#   define UCONFIG_NO_COLLATION 1
#   define UCONFIG_NO_TRANSLITERATION 1
#endif







#ifndef UCONFIG_NO_BREAK_ITERATION
#   define UCONFIG_NO_BREAK_ITERATION 0
#endif







#ifndef UCONFIG_NO_IDNA
#   define UCONFIG_NO_IDNA 0
#endif








#ifndef UCONFIG_MSGPAT_DEFAULT_APOSTROPHE_MODE
#   define UCONFIG_MSGPAT_DEFAULT_APOSTROPHE_MODE UMSGPAT_APOS_DOUBLE_OPTIONAL
#endif









#ifndef UCONFIG_NO_COLLATION
#   define UCONFIG_NO_COLLATION 0
#endif







#ifndef UCONFIG_NO_FORMATTING
#   define UCONFIG_NO_FORMATTING 0
#endif







#ifndef UCONFIG_NO_TRANSLITERATION
#   define UCONFIG_NO_TRANSLITERATION 0
#endif







#ifndef UCONFIG_NO_REGULAR_EXPRESSIONS
#   define UCONFIG_NO_REGULAR_EXPRESSIONS 0
#endif







#ifndef UCONFIG_NO_SERVICE
#   define UCONFIG_NO_SERVICE 0
#endif







#ifndef UCONFIG_HAVE_PARSEALLINPUT
#   define UCONFIG_HAVE_PARSEALLINPUT 1
#endif








#ifndef UCONFIG_FORMAT_FASTPATHS_49
#   define UCONFIG_FORMAT_FASTPATHS_49 1
#endif

#endif
