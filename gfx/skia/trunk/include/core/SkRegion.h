








#ifndef SkRegion_DEFINED
#define SkRegion_DEFINED

#include "SkRect.h"

class SkPath;
class SkRgnBuilder;

namespace android {
    class Region;
}

#define SkRegion_gEmptyRunHeadPtr   ((SkRegion::RunHead*)-1)
#define SkRegion_gRectRunHeadPtr    0






class SK_API SkRegion {
public:
    typedef int32_t RunType;
    enum {
        kRunTypeSentinel = 0x7FFFFFFF
    };

    SkRegion();
    SkRegion(const SkRegion&);
    explicit SkRegion(const SkIRect&);
    ~SkRegion();

    SkRegion& operator=(const SkRegion&);

    



    bool operator==(const SkRegion& other) const;

    


    bool operator!=(const SkRegion& other) const {
        return !(*this == other);
    }

    



    bool set(const SkRegion& src) {
        SkASSERT(&src);
        *this = src;
        return !this->isEmpty();
    }

    



    void swap(SkRegion&);

    
    bool isEmpty() const { return fRunHead == SkRegion_gEmptyRunHeadPtr; }

    
    bool isRect() const { return fRunHead == SkRegion_gRectRunHeadPtr; }

    
    bool isComplex() const { return !this->isEmpty() && !this->isRect(); }

    



    const SkIRect& getBounds() const { return fBounds; }

    







    int computeRegionComplexity() const;

    




    bool getBoundaryPath(SkPath* path) const;

    



    bool setEmpty();

    



    bool setRect(const SkIRect&);

    



    bool setRect(int32_t left, int32_t top, int32_t right, int32_t bottom);

    





    bool setRects(const SkIRect rects[], int count);

    



    bool setRegion(const SkRegion&);

    





    bool setPath(const SkPath&, const SkRegion& clip);

    



    bool intersects(const SkIRect&) const;

    



    bool intersects(const SkRegion&) const;

    


    bool contains(int32_t x, int32_t y) const;

    





    bool contains(const SkIRect&) const;

    





    bool contains(const SkRegion&) const;

    





    bool quickContains(const SkIRect& r) const {
        return this->quickContains(r.fLeft, r.fTop, r.fRight, r.fBottom);
    }

    






    bool quickContains(int32_t left, int32_t top, int32_t right,
                       int32_t bottom) const {
        SkASSERT(this->isEmpty() == fBounds.isEmpty()); 

        return left < right && top < bottom &&
               fRunHead == SkRegion_gRectRunHeadPtr &&  
               
               fBounds.fLeft <= left && fBounds.fTop <= top &&
               fBounds.fRight >= right && fBounds.fBottom >= bottom;
    }

    




    bool quickReject(const SkIRect& rect) const {
        return this->isEmpty() || rect.isEmpty() ||
                !SkIRect::Intersects(fBounds, rect);
    }

    




    bool quickReject(const SkRegion& rgn) const {
        return this->isEmpty() || rgn.isEmpty() ||
               !SkIRect::Intersects(fBounds, rgn.fBounds);
    }

    
    void translate(int dx, int dy) { this->translate(dx, dy, this); }

    





    void translate(int dx, int dy, SkRegion* dst) const;

    


    enum Op {
        kDifference_Op, 
        kIntersect_Op,  
        kUnion_Op,      
        kXOR_Op,        
        
        kReverseDifference_Op,
        kReplace_Op,    

        kLastOp = kReplace_Op
    };

    static const int kOpCnt = kLastOp + 1;

    




    bool op(const SkIRect& rect, Op op) { return this->op(*this, rect, op); }

    




    bool op(int left, int top, int right, int bottom, Op op) {
        SkIRect rect;
        rect.set(left, top, right, bottom);
        return this->op(*this, rect, op);
    }

    




    bool op(const SkRegion& rgn, Op op) { return this->op(*this, rgn, op); }

    




    bool op(const SkIRect& rect, const SkRegion& rgn, Op);

    




    bool op(const SkRegion& rgn, const SkIRect& rect, Op);

    




    bool op(const SkRegion& rgna, const SkRegion& rgnb, Op op);

#ifdef SK_BUILD_FOR_ANDROID
    

    char* toString();
#endif

    



    class SK_API Iterator {
    public:
        Iterator() : fRgn(NULL), fDone(true) {}
        Iterator(const SkRegion&);
        
        bool rewind();
        
        void reset(const SkRegion&);
        bool done() const { return fDone; }
        void next();
        const SkIRect& rect() const { return fRect; }
        
        const SkRegion* rgn() const { return fRgn; }

    private:
        const SkRegion* fRgn;
        const RunType*  fRuns;
        SkIRect         fRect;
        bool            fDone;
    };

    



    class SK_API Cliperator {
    public:
        Cliperator(const SkRegion&, const SkIRect& clip);
        bool done() { return fDone; }
        void  next();
        const SkIRect& rect() const { return fRect; }

    private:
        Iterator    fIter;
        SkIRect     fClip;
        SkIRect     fRect;
        bool        fDone;
    };

    



    class Spanerator {
    public:
        Spanerator(const SkRegion&, int y, int left, int right);
        bool next(int* left, int* right);

    private:
        const SkRegion::RunType* fRuns;
        int     fLeft, fRight;
        bool    fDone;
    };

    



    size_t writeToMemory(void* buffer) const;
    







    size_t readFromMemory(const void* buffer, size_t length);

    



    static const SkRegion& GetEmptyRegion();

    SkDEBUGCODE(void dump() const;)
    SkDEBUGCODE(void validate() const;)
    SkDEBUGCODE(static void UnitTest();)

    
    SkDEBUGCODE(bool debugSetRuns(const RunType runs[], int count);)

private:
    enum {
        kOpCount = kReplace_Op + 1
    };

    enum {
        
        
        
        kRectRegionRuns = 7
    };

    friend class android::Region;    

    struct RunHead;

    
    void allocateRuns(int count);
    void allocateRuns(int count, int ySpanCount, int intervalCount);
    void allocateRuns(const RunHead& src);

    SkIRect     fBounds;
    RunHead*    fRunHead;

    void freeRuns();

    




    const RunType*  getRuns(RunType tmpStorage[], int* intervals) const;

    
    
    
    bool setRuns(RunType runs[], int count);

    int count_runtype_values(int* itop, int* ibot) const;

    static void BuildRectRuns(const SkIRect& bounds,
                              RunType runs[kRectRegionRuns]);

    
    
    static bool RunsAreARect(const SkRegion::RunType runs[], int count,
                             SkIRect* bounds);

    



    static bool Oper(const SkRegion&, const SkRegion&, SkRegion::Op, SkRegion*);

    friend struct RunHead;
    friend class Iterator;
    friend class Spanerator;
    friend class SkRgnBuilder;
    friend class SkFlatRegion;
};

#endif
