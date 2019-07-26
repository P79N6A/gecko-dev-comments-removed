









#ifndef SkTypefaceCache_DEFINED
#define SkTypefaceCache_DEFINED

#include "SkTypeface.h"
#include "SkTDArray.h"








class SkTypefaceCache {
public:
    SkTypefaceCache();
    ~SkTypefaceCache();

    




    typedef bool (*FindProc)(SkTypeface*, SkTypeface::Style, void* context);

    





    void add(SkTypeface*, SkTypeface::Style requested, bool strong = true);

    





    SkTypeface* findByID(SkFontID findID) const;

    




    SkTypeface* findByProcAndRef(FindProc proc, void* ctx) const;

    





    void purgeAll();

    



    static SkFontID NewFontID();

    

    static void Add(SkTypeface*,
                    SkTypeface::Style requested,
                    bool strong = true);
    static SkTypeface* FindByID(SkFontID fontID);
    static SkTypeface* FindByProcAndRef(FindProc proc, void* ctx);
    static void PurgeAll();

    


    static void Dump();

private:
    static SkTypefaceCache& Get();

    void purge(int count);

    struct Rec {
        SkTypeface*         fFace;
        bool                fStrong;
        SkTypeface::Style   fRequestedStyle;
    };
    SkTDArray<Rec> fArray;
};

#endif
