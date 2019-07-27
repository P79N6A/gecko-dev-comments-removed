






#ifndef SkFontConfigInterface_DEFINED
#define SkFontConfigInterface_DEFINED

#include "SkDataTable.h"
#include "SkFontStyle.h"
#include "SkRefCnt.h"
#include "SkTArray.h"
#include "SkTypeface.h"

struct SkBaseMutex;







class SK_API SkFontConfigInterface : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkFontConfigInterface)

    




    static SkFontConfigInterface* RefGlobal();

    




    static SkFontConfigInterface* SetGlobal(SkFontConfigInterface*);

    




    struct FontIdentity {
        FontIdentity() : fID(0), fTTCIndex(0) {}

        bool operator==(const FontIdentity& other) const {
            return fID == other.fID &&
                   fTTCIndex == other.fTTCIndex &&
                   fString == other.fString;
        }
        bool operator!=(const FontIdentity& other) const {
            return !(*this == other);
        }

        uint32_t    fID;
        int32_t     fTTCIndex;
        SkString    fString;
        SkFontStyle fStyle;

        
        
        size_t writeToMemory(void* buffer = NULL) const;

        
        size_t readFromMemory(const void* buffer, size_t length);
    };

    










    virtual bool matchFamilyName(const char familyName[],
                                 SkTypeface::Style requested,
                                 FontIdentity* outFontIdentifier,
                                 SkString* outFamilyName,
                                 SkTypeface::Style* outStyle) = 0;

    




    virtual SkStream* openStream(const FontIdentity&) = 0;

    




    static SkFontConfigInterface* GetSingletonDirectInterface
        (SkBaseMutex* mutex = NULL);

    

    virtual SkDataTable* getFamilyNames() { return SkDataTable::NewEmpty(); }
    virtual bool matchFamilySet(const char inFamilyName[],
                                SkString* outFamilyName,
                                SkTArray<FontIdentity>*) {
        return false;
    }
    typedef SkRefCnt INHERITED;
};

#endif
