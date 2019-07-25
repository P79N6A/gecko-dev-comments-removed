








#ifndef SkGlobals_DEFINED
#define SkGlobals_DEFINED

#include "SkThread.h"

class SkGlobals {
public:
    class Rec {
    public:
        virtual ~Rec();
    private:
        Rec*        fNext;
        uint32_t    fTag;

        friend class SkGlobals;
    };

    






    static Rec* Find(uint32_t tag, Rec* (*create_proc)());
    

    static Rec* Get(uint32_t tag)
    {
        Rec* rec = SkGlobals::Find(tag, NULL);
        SkASSERT(rec);
        return rec;
    }

    
    struct BootStrap {
        SkMutex fMutex;
        Rec*    fHead;
    };

private:
    static void Init();
    static void Term();
    friend class SkGraphics;

    
    static BootStrap& GetBootStrap();
};

#endif

