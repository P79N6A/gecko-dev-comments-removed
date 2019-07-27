






#ifndef SkFontMgr_DEFINED
#define SkFontMgr_DEFINED

#include "SkRefCnt.h"
#include "SkFontStyle.h"

class SkData;
class SkStream;
class SkString;
class SkTypeface;

class SK_API SkFontStyleSet : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkFontStyleSet)

    virtual int count() = 0;
    virtual void getStyle(int index, SkFontStyle*, SkString* style) = 0;
    virtual SkTypeface* createTypeface(int index) = 0;
    virtual SkTypeface* matchStyle(const SkFontStyle& pattern) = 0;

    static SkFontStyleSet* CreateEmpty();

private:
    typedef SkRefCnt INHERITED;
};

class SkTypeface;

class SK_API SkFontMgr : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkFontMgr)

    int countFamilies() const;
    void getFamilyName(int index, SkString* familyName) const;
    SkFontStyleSet* createStyleSet(int index) const;

    






    SkFontStyleSet* matchFamily(const char familyName[]) const;

    









    SkTypeface* matchFamilyStyle(const char familyName[], const SkFontStyle&) const;

    







    SkTypeface* matchFamilyStyleCharacter(const char familyName[], const SkFontStyle&,
                                          const char bpc47[], uint32_t character) const;

    SkTypeface* matchFaceStyle(const SkTypeface*, const SkFontStyle&) const;

    




    SkTypeface* createFromData(SkData*, int ttcIndex = 0) const;

    




    SkTypeface* createFromStream(SkStream*, int ttcIndex = 0) const;

    





    SkTypeface* createFromFile(const char path[], int ttcIndex = 0) const;

    SkTypeface* legacyCreateTypeface(const char familyName[],
                                     unsigned typefaceStyleBits) const;

    



    static SkFontMgr* RefDefault();

protected:
    virtual int onCountFamilies() const = 0;
    virtual void onGetFamilyName(int index, SkString* familyName) const = 0;
    virtual SkFontStyleSet* onCreateStyleSet(int index)const  = 0;

    
    virtual SkFontStyleSet* onMatchFamily(const char familyName[]) const = 0;

    virtual SkTypeface* onMatchFamilyStyle(const char familyName[],
                                           const SkFontStyle&) const = 0;
    
    virtual SkTypeface* onMatchFamilyStyleCharacter(const char familyName[], const SkFontStyle&,
                                                    const char bpc47[], uint32_t character) const
    { return NULL; }
    virtual SkTypeface* onMatchFaceStyle(const SkTypeface*,
                                         const SkFontStyle&) const = 0;

    virtual SkTypeface* onCreateFromData(SkData*, int ttcIndex) const = 0;
    virtual SkTypeface* onCreateFromStream(SkStream*, int ttcIndex) const = 0;
    virtual SkTypeface* onCreateFromFile(const char path[], int ttcIndex) const = 0;

    virtual SkTypeface* onLegacyCreateTypeface(const char familyName[],
                                               unsigned styleBits) const = 0;
private:
    static SkFontMgr* Factory();    
    static SkFontMgr* CreateDefault();

    typedef SkRefCnt INHERITED;
};

#endif
