









#ifndef SkTypefaceCache_DEFINED
#define SkTypefaceCache_DEFINED

#include "SkTypeface.h"
#include "SkTDArray.h"








class SkTypefaceCache {
public:
    typedef bool (*FindProc)(SkTypeface*, SkTypeface::Style, void* context);

    



    static SkFontID NewFontID();

    




    static void Add(SkTypeface*, SkTypeface::Style requested);

    




    static SkTypeface* FindByID(SkFontID fontID);

    




    static SkTypeface* FindByProc(FindProc proc, void* ctx);

    




    static void PurgeAll();

    


    static void Dump();

private:
    static SkTypefaceCache& Get();

    void add(SkTypeface*, SkTypeface::Style requested);
    SkTypeface* findByID(SkFontID findID) const;
    SkTypeface* findByProc(FindProc proc, void* ctx) const;
    void purge(int count);
    void purgeAll();

    struct Rec {
        SkTypeface*         fFace;
        SkTypeface::Style   fRequestedStyle;
    };
    SkTDArray<Rec> fArray;
};

#endif
