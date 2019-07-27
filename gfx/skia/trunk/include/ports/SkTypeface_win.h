






#ifndef SkTypeface_win_DEFINED
#define SkTypeface_win_DEFINED

#include "SkTypeface.h"






SK_API SkTypeface* SkCreateTypefaceFromLOGFONT(const LOGFONT&);







SK_API void SkLOGFONTFromTypeface(const SkTypeface* typeface, LOGFONT* lf);







SK_API void SkTypeface_SetEnsureLOGFONTAccessibleProc(void (*)(const LOGFONT&));



class SkFontMgr;
class SkRemotableFontMgr;
struct IDWriteFactory;

SK_API SkFontMgr* SkFontMgr_New_GDI();
SK_API SkFontMgr* SkFontMgr_New_DirectWrite(IDWriteFactory* factory = NULL);







SK_API SkFontMgr* SkFontMgr_New_DirectWriteRenderer(SkRemotableFontMgr*);







SK_API SkRemotableFontMgr* SkRemotableFontMgr_New_DirectWrite();

#endif
