






#ifndef SkDWrite_DEFINED
#define SkDWrite_DEFINED

#include "SkTemplates.h"

#include <dwrite.h>

class SkString;




IDWriteFactory* sk_get_dwrite_factory();





typedef SkAutoSTMalloc<16, WCHAR> SkSMallocWCHAR;


HRESULT sk_cstring_to_wchar(const char* skname, SkSMallocWCHAR* name);


HRESULT sk_wchar_to_skstring(WCHAR* name, SkString* skname);




void sk_get_locale_string(IDWriteLocalizedStrings* names, const WCHAR* preferedLocale,
                       SkString* skname);

typedef decltype(GetUserDefaultLocaleName)* SkGetUserDefaultLocaleNameProc;
HRESULT SkGetGetUserDefaultLocaleNameProc(SkGetUserDefaultLocaleNameProc* proc);




class AutoDWriteTable {
public:
    AutoDWriteTable(IDWriteFontFace* fontFace, UINT32 beTag) : fFontFace(fontFace), fExists(FALSE) {
        
        fontFace->TryGetFontTable(beTag,
            reinterpret_cast<const void **>(&fData), &fSize, &fLock, &fExists);
    }
    ~AutoDWriteTable() {
        if (fExists) {
            fFontFace->ReleaseFontTable(fLock);
        }
    }

    const uint8_t* fData;
    UINT32 fSize;
    BOOL fExists;
private:
    
    IDWriteFontFace* fFontFace;
    void* fLock;
};
template<typename T> class AutoTDWriteTable : public AutoDWriteTable {
public:
    static const UINT32 tag = DWRITE_MAKE_OPENTYPE_TAG(T::TAG0, T::TAG1, T::TAG2, T::TAG3);
    AutoTDWriteTable(IDWriteFontFace* fontFace) : AutoDWriteTable(fontFace, tag) { }

    const T* get() const { return reinterpret_cast<const T*>(fData); }
    const T* operator->() const { return reinterpret_cast<const T*>(fData); }
};

#endif
