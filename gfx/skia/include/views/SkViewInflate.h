








#ifndef SkViewInflate_DEFINED
#define SkViewInflate_DEFINED

#include "SkDOM.h"
#include "SkTDict.h"
#include "SkEvent.h"

class SkView;

class SkViewInflate {
public: 
            SkViewInflate();
    virtual ~SkViewInflate();

    




    SkView* inflate(const SkDOM& dom, const SkDOM::Node* node, SkView* root = NULL);
    SkView* inflate(const char xml[], size_t len, SkView* root = NULL);

    


    SkView* findViewByID(const char id[]) const;
    
    SkDEBUGCODE(void dump() const;)

protected:
    





    virtual SkView* createView(const SkDOM& dom, const SkDOM::Node* node);
    



    virtual void inflateView(SkView* view, const SkDOM& dom, const SkDOM::Node* node);

private:
    enum {
        kMinIDStrAlloc = 64
    };
    SkTDict<SkView*> fIDs;

    struct IDStr {
        SkView* fView;
        char*   fStr;
    };
    SkTDArray<IDStr>    fListenTo, fBroadcastTo;
    SkChunkAlloc        fStrings;

    void addIDStr(SkTDArray<IDStr>* list, SkView*, const char* str);

    void    rInflate(const SkDOM& dom, const SkDOM::Node* node, SkView* parent);
};

#endif

