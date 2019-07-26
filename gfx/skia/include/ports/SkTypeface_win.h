









#ifndef SkTypeface_win_DEFINED
#define SkTypeface_win_DEFINED

#include "SkTypeface.h"






SK_API SkTypeface* SkCreateTypefaceFromLOGFONT(const LOGFONT&);







SK_API void SkLOGFONTFromTypeface(const SkTypeface* typeface, LOGFONT* lf);







SK_API void SkTypeface_SetEnsureLOGFONTAccessibleProc(void (*)(const LOGFONT&));

#endif
