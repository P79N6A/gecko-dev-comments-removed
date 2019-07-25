






#ifndef SkClipStack_DEFINED
#define SkClipStack_DEFINED

#include "SkDeque.h"
#include "SkRegion.h"

struct SkRect;
class SkPath;

class SK_API SkClipStack {
public:
    SkClipStack();
    SkClipStack(const SkClipStack& b);
    ~SkClipStack() {}

    SkClipStack& operator=(const SkClipStack& b);
    bool operator==(const SkClipStack& b) const;
    bool operator!=(const SkClipStack& b) const { return !(*this == b); }

    void reset();

    int getSaveCount() const { return fSaveCount; }
    void save();
    void restore();

    void clipDevRect(const SkIRect& ir,
                     SkRegion::Op op = SkRegion::kIntersect_Op) {
        SkRect r;
        r.set(ir);
        this->clipDevRect(r, op, false);
    }
    void clipDevRect(const SkRect&, SkRegion::Op, bool doAA);
    void clipDevPath(const SkPath&, SkRegion::Op, bool doAA);

    class B2FIter {
    public:
        


        B2FIter();

        B2FIter(const SkClipStack& stack);

        struct Clip {
            Clip() : fRect(NULL), fPath(NULL), fOp(SkRegion::kIntersect_Op) {}
            friend bool operator==(const Clip& a, const Clip& b);
            friend bool operator!=(const Clip& a, const Clip& b);
            const SkRect*   fRect;  
            const SkPath*   fPath;  
            SkRegion::Op    fOp;
            bool            fDoAA;
        };

        








        const Clip* next();

        


        void reset(const SkClipStack& stack);

    private:
        Clip             fClip;
        SkDeque::F2BIter fIter;
    };

private:
    friend class B2FIter;
    struct Rec;

    SkDeque fDeque;
    int     fSaveCount;
};

#endif

