






#ifndef SkClipStack_DEFINED
#define SkClipStack_DEFINED

#include "SkDeque.h"
#include "SkRegion.h"
#include "SkTDArray.h"

struct SkRect;
class SkPath;







class SK_API SkClipStack {
public:
    SkClipStack();
    SkClipStack(const SkClipStack& b);
    explicit SkClipStack(const SkRect& r);
    explicit SkClipStack(const SkIRect& r);
    ~SkClipStack();

    SkClipStack& operator=(const SkClipStack& b);
    bool operator==(const SkClipStack& b) const;
    bool operator!=(const SkClipStack& b) const { return !(*this == b); }

    void reset();

    int getSaveCount() const { return fSaveCount; }
    void save();
    void restore();

    enum BoundsType {
        
        kNormal_BoundsType,
        
        
        
        
        
        kInsideOut_BoundsType
    };

    








    void getBounds(SkRect* canvFiniteBound,
                   BoundsType* boundType,
                   bool* isIntersectionOfRects = NULL) const;

    void clipDevRect(const SkIRect& ir, SkRegion::Op op) {
        SkRect r;
        r.set(ir);
        this->clipDevRect(r, op, false);
    }
    void clipDevRect(const SkRect&, SkRegion::Op, bool doAA);
    void clipDevPath(const SkPath&, SkRegion::Op, bool doAA);
    
    void clipEmpty();

    



    bool isWideOpen() const;

    






    typedef void (*PFPurgeClipCB)(int genID, void* data);
    void addPurgeClipCallback(PFPurgeClipCB callback, void* data) const;

    


    void removePurgeClipCallback(PFPurgeClipCB callback, void* data) const;

    



    static const int32_t kInvalidGenID = 0;
    static const int32_t kEmptyGenID = 1;       
    static const int32_t kWideOpenGenID = 2;    

    int32_t getTopmostGenID() const;

private:
    struct Rec;

public:
    class Iter {
    public:
        enum IterStart {
            kBottom_IterStart = SkDeque::Iter::kFront_IterStart,
            kTop_IterStart = SkDeque::Iter::kBack_IterStart
        };

        


        Iter();

        Iter(const SkClipStack& stack, IterStart startLoc);

        struct Clip {
            Clip() : fRect(NULL), fPath(NULL), fOp(SkRegion::kIntersect_Op),
                     fDoAA(false) {}
            friend bool operator==(const Clip& a, const Clip& b);
            friend bool operator!=(const Clip& a, const Clip& b);
            const SkRect*   fRect;  
            const SkPath*   fPath;  
            SkRegion::Op    fOp;
            bool            fDoAA;
            int32_t         fGenID;
        };

        








        const Clip* next();
        const Clip* prev();

        




        const Clip* skipToTopmost(SkRegion::Op op);

        


        void reset(const SkClipStack& stack, IterStart startLoc);

    private:
        const SkClipStack* fStack;
        Clip               fClip;
        SkDeque::Iter      fIter;

        



        const Clip* updateClip(const SkClipStack::Rec* rec);
    };

    



    class B2TIter : private Iter {
    public:
        B2TIter() {}

        



        B2TIter(const SkClipStack& stack)
        : INHERITED(stack, kBottom_IterStart) {
        }

        using Iter::Clip;
        using Iter::next;

        



        void reset(const SkClipStack& stack) {
            this->INHERITED::reset(stack, kBottom_IterStart);
        }

    private:

        typedef Iter INHERITED;
    };

    












    void getConservativeBounds(int offsetX,
                               int offsetY,
                               int maxWidth,
                               int maxHeight,
                               SkRect* devBounds,
                               bool* isIntersectionOfRects = NULL) const;

private:
    friend class Iter;

    SkDeque fDeque;
    int     fSaveCount;

    
    
    
    static int32_t     gGenID;

    struct ClipCallbackData {
        PFPurgeClipCB   fCallback;
        void*           fData;

        friend bool operator==(const ClipCallbackData& a,
                               const ClipCallbackData& b) {
            return a.fCallback == b.fCallback && a.fData == b.fData;
        }
    };

    mutable SkTDArray<ClipCallbackData> fCallbackData;

    


    void purgeClip(Rec* rec);

    


    static int32_t GetNextGenID();
};

#endif

