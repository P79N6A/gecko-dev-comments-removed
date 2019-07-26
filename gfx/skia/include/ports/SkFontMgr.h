






#ifndef SkFontMgr_DEFINED
#define SkFontMgr_DEFINED

#include "SkRefCnt.h"
#include "SkFontStyle.h"

class SkData;
class SkStream;
class SkString;

class SkFontStyleSet : public SkRefCnt {
public:
    virtual int count() = 0;
    virtual void getStyle(int index, SkFontStyle*, SkString* style) = 0;
    virtual SkTypeface* createTypeface(int index) = 0;
    virtual SkTypeface* matchStyle(const SkFontStyle& pattern) = 0;

    static SkFontStyleSet* CreateEmpty();
};

class SkFontMgr : public SkRefCnt {
public:
    int countFamilies();
    void getFamilyName(int index, SkString* familyName);
    SkFontStyleSet* createStyleSet(int index);

    SkFontStyleSet* matchFamily(const char familyName[]);

    





    SkTypeface* matchFamilyStyle(const char familyName[], const SkFontStyle&);

    SkTypeface* matchFaceStyle(const SkTypeface*, const SkFontStyle&);

    




    SkTypeface* createFromData(SkData*, int ttcIndex = 0);

    




    SkTypeface* createFromStream(SkStream*, int ttcIndex = 0);

    





    SkTypeface* createFromFile(const char path[], int ttcIndex = 0);

    



    static SkFontMgr* RefDefault();

protected:
    virtual int onCountFamilies() = 0;
    virtual void onGetFamilyName(int index, SkString* familyName) = 0;
    virtual SkFontStyleSet* onCreateStyleSet(int index) = 0;

    virtual SkFontStyleSet* onMatchFamily(const char familyName[]) = 0;

    virtual SkTypeface* onMatchFamilyStyle(const char familyName[],
                                           const SkFontStyle&) = 0;
    virtual SkTypeface* onMatchFaceStyle(const SkTypeface*,
                                         const SkFontStyle&) = 0;

    virtual SkTypeface* onCreateFromData(SkData*, int ttcIndex) = 0;
    virtual SkTypeface* onCreateFromStream(SkStream*, int ttcIndex) = 0;
    virtual SkTypeface* onCreateFromFile(const char path[], int ttcIndex) = 0;

private:
    static SkFontMgr* Factory();    
    static SkMutex* Mutex();        

    typedef SkRefCnt INHERITED;
};

#endif
