






#ifndef SkFontConfigInterface_DEFINED
#define SkFontConfigInterface_DEFINED

#include "SkRefCnt.h"
#include "SkTypeface.h"







class SK_API SkFontConfigInterface : public SkRefCnt {
public:
    




    static SkFontConfigInterface* RefGlobal();

    




    static SkFontConfigInterface* SetGlobal(SkFontConfigInterface*);

    




    struct FontIdentity {
        FontIdentity() : fID(0), fTTCIndex(0) {}

        bool operator==(const FontIdentity& other) const {
            return fID == other.fID &&
                   fTTCIndex == other.fTTCIndex &&
                   fString == other.fString;
        }

        uint32_t    fID;
        int32_t     fTTCIndex;
        SkString    fString;
    };

    










    virtual bool matchFamilyName(const char familyName[],
                                 SkTypeface::Style requested,
                                 FontIdentity* outFontIdentifier,
                                 SkString* outFamilyName,
                                 SkTypeface::Style* outStyle) = 0;

    




    virtual SkStream* openStream(const FontIdentity&) = 0;

    



    static SkFontConfigInterface* GetSingletonDirectInterface();
};

#endif
