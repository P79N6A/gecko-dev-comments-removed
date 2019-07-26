





#ifdef std
#  undef std /* We undef "std" on entry , as STLport headers may include native ones. */
#endif

#ifdef _STLP_PROLOG_HEADER_INCLUDED
#  error STlport prolog header can not be reincluded as long as epilog has not be included.
#endif

#define _STLP_PROLOG_HEADER_INCLUDED

#ifndef _STLP_FEATURES_H
#  include <stl/config/features.h>
#endif



#if defined (_STLP_HAS_SPECIFIC_PROLOG_EPILOG)
#  include <stl/config/_prolog.h>
#endif
