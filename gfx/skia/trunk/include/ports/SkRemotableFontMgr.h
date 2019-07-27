






#ifndef SkRemotableFontMgr_DEFINED
#define SkRemotableFontMgr_DEFINED

#include "SkFontStyle.h"
#include "SkRefCnt.h"
#include "SkTemplates.h"

class SkDataTable;
class SkStreamAsset;
class SkString;

struct SK_API SkFontIdentity {
    static const uint32_t kInvalidDataId = 0xFFFFFFFF;

    
    
    uint32_t fDataId;
    uint32_t fTtcIndex;

    
    
    
    
    
    
    SkFontStyle fFontStyle;
};

class SK_API SkRemotableFontIdentitySet : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkRemotableFontIdentitySet)

    SkRemotableFontIdentitySet(int count, SkFontIdentity** data);

    int count() const { return fCount; }
    const SkFontIdentity& at(int index) const { return fData[index]; }

    static SkRemotableFontIdentitySet* NewEmpty();

private:
    SkRemotableFontIdentitySet() : fCount(0), fData() { }
    static SkRemotableFontIdentitySet* NewEmptyImpl();

    int fCount;
    SkAutoTMalloc<SkFontIdentity> fData;

    typedef SkRefCnt INHERITED;
};

class SK_API SkRemotableFontMgr : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkRemotableFontMgr)

    








    virtual SkDataTable* getFamilyNames() const = 0;

    






    virtual SkRemotableFontIdentitySet* getIndex(int familyIndex) const = 0;

    




    virtual SkFontIdentity matchIndexStyle(int familyIndex, const SkFontStyle&) const = 0;

    
















    virtual SkRemotableFontIdentitySet* matchName(const char familyName[]) const = 0;

    
















    virtual SkFontIdentity matchNameStyle(const char familyName[], const SkFontStyle&) const = 0;

    









    virtual SkFontIdentity matchNameStyleCharacter(const char familyName[], const SkFontStyle&,
                                                   const char bpc47[], SkUnichar character) const=0;

    






    virtual SkStreamAsset* getData(int dataId) const = 0;

private:
    typedef SkRefCnt INHERITED;
};

#endif
