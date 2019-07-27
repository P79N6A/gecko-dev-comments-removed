








#ifndef SkPath_DEFINED
#define SkPath_DEFINED

#include "SkInstCnt.h"
#include "SkMatrix.h"
#include "SkPathRef.h"
#include "SkTDArray.h"
#include "SkRefCnt.h"

class SkReader32;
class SkWriter32;
class SkAutoPathBoundsUpdate;
class SkString;
class SkRRect;
class SkWStream;






class SK_API SkPath {
public:
    SK_DECLARE_INST_COUNT_ROOT(SkPath);

    SkPath();
    SkPath(const SkPath&);
    ~SkPath();

    SkPath& operator=(const SkPath&);
    friend  SK_API bool operator==(const SkPath&, const SkPath&);
    friend bool operator!=(const SkPath& a, const SkPath& b) {
        return !(a == b);
    }

    enum FillType {
        


        kWinding_FillType,
        


        kEvenOdd_FillType,
        

        kInverseWinding_FillType,
        

        kInverseEvenOdd_FillType
    };

    




    FillType getFillType() const { return (FillType)fFillType; }

    




    void setFillType(FillType ft) {
        fFillType = SkToU8(ft);
    }

    
    bool isInverseFillType() const { return IsInverseFillType((FillType)fFillType); }

    



    void toggleInverseFillType() {
        fFillType ^= 2;
    }

    enum Convexity {
        kUnknown_Convexity,
        kConvex_Convexity,
        kConcave_Convexity
    };

    



    Convexity getConvexity() const {
        if (kUnknown_Convexity != fConvexity) {
            return static_cast<Convexity>(fConvexity);
        } else {
            return this->internalGetConvexity();
        }
    }

    





    Convexity getConvexityOrUnknown() const { return (Convexity)fConvexity; }

    








    void setConvexity(Convexity);

    



    bool isConvex() const {
        return kConvex_Convexity == this->getConvexity();
    }

    





    SK_ATTR_DEPRECATED("use setConvexity")
    void setIsConvex(bool isConvex) {
        this->setConvexity(isConvex ? kConvex_Convexity : kConcave_Convexity);
    }

    









    bool isOval(SkRect* rect) const { return fPathRef->isOval(rect); }

    



    void reset();

    




    void rewind();

    



    bool isEmpty() const {
        SkDEBUGCODE(this->validate();)
        return 0 == fPathRef->countVerbs();
    }

    



    bool isFinite() const {
        SkDEBUGCODE(this->validate();)
        return fPathRef->isFinite();
    }

    



    static bool IsLineDegenerate(const SkPoint& p1, const SkPoint& p2) {
        return p1.equalsWithinTolerance(p2);
    }

    



    static bool IsQuadDegenerate(const SkPoint& p1, const SkPoint& p2,
                                 const SkPoint& p3) {
        return p1.equalsWithinTolerance(p2) &&
               p2.equalsWithinTolerance(p3);
    }

    



    static bool IsCubicDegenerate(const SkPoint& p1, const SkPoint& p2,
                                  const SkPoint& p3, const SkPoint& p4) {
        return p1.equalsWithinTolerance(p2) &&
               p2.equalsWithinTolerance(p3) &&
               p3.equalsWithinTolerance(p4);
    }

    





    bool isLine(SkPoint line[2]) const;

    







    bool isRect(SkRect* rect) const;

    

    int countPoints() const;

    



    SkPoint getPoint(int index) const;

    





    int getPoints(SkPoint points[], int max) const;

    

    int countVerbs() const;

    






    int getVerbs(uint8_t verbs[], int max) const;

    
    void swap(SkPath& other);

    




    const SkRect& getBounds() const {
        return fPathRef->getBounds();
    }

    




    void updateBoundsCache() const {
        
        this->getBounds();
    }

    





    bool conservativelyContainsRect(const SkRect& rect) const;

    

    





    void incReserve(unsigned extraPtCount);

    




    void moveTo(SkScalar x, SkScalar y);

    



    void moveTo(const SkPoint& p) {
        this->moveTo(p.fX, p.fY);
    }

    








    void rMoveTo(SkScalar dx, SkScalar dy);

    






    void lineTo(SkScalar x, SkScalar y);

    





    void lineTo(const SkPoint& p) {
        this->lineTo(p.fX, p.fY);
    }

    








    void rLineTo(SkScalar dx, SkScalar dy);

    








    void quadTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2);

    






    void quadTo(const SkPoint& p1, const SkPoint& p2) {
        this->quadTo(p1.fX, p1.fY, p2.fX, p2.fY);
    }

    












    void rQuadTo(SkScalar dx1, SkScalar dy1, SkScalar dx2, SkScalar dy2);

    void conicTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2,
                 SkScalar w);
    void conicTo(const SkPoint& p1, const SkPoint& p2, SkScalar w) {
        this->conicTo(p1.fX, p1.fY, p2.fX, p2.fY, w);
    }
    void rConicTo(SkScalar dx1, SkScalar dy1, SkScalar dx2, SkScalar dy2,
                  SkScalar w);

    










    void cubicTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2,
                 SkScalar x3, SkScalar y3);

    







    void cubicTo(const SkPoint& p1, const SkPoint& p2, const SkPoint& p3) {
        this->cubicTo(p1.fX, p1.fY, p2.fX, p2.fY, p3.fX, p3.fY);
    }

    
















    void rCubicTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2,
                  SkScalar x3, SkScalar y3);

    











    void arcTo(const SkRect& oval, SkScalar startAngle, SkScalar sweepAngle,
               bool forceMoveTo);

    


    void arcTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2,
               SkScalar radius);

    


    void arcTo(const SkPoint p1, const SkPoint p2, SkScalar radius) {
        this->arcTo(p1.fX, p1.fY, p2.fX, p2.fY, radius);
    }

    


    void close();

    enum Direction {
        
        kUnknown_Direction,
        
        kCW_Direction,
        
        kCCW_Direction,
    };

    



    static Direction OppositeDirection(Direction dir) {
        static const Direction gOppositeDir[] = {
            kUnknown_Direction, kCCW_Direction, kCW_Direction
        };
        return gOppositeDir[dir];
    }

    







    static bool IsInverseFillType(FillType fill) {
        SK_COMPILE_ASSERT(0 == kWinding_FillType, fill_type_mismatch);
        SK_COMPILE_ASSERT(1 == kEvenOdd_FillType, fill_type_mismatch);
        SK_COMPILE_ASSERT(2 == kInverseWinding_FillType, fill_type_mismatch);
        SK_COMPILE_ASSERT(3 == kInverseEvenOdd_FillType, fill_type_mismatch);
        return (fill & 2) != 0;
    }

    







    static FillType ConvertToNonInverseFillType(FillType fill) {
        SK_COMPILE_ASSERT(0 == kWinding_FillType, fill_type_mismatch);
        SK_COMPILE_ASSERT(1 == kEvenOdd_FillType, fill_type_mismatch);
        SK_COMPILE_ASSERT(2 == kInverseWinding_FillType, fill_type_mismatch);
        SK_COMPILE_ASSERT(3 == kInverseEvenOdd_FillType, fill_type_mismatch);
        return (FillType)(fill & 1);
    }

    






    bool cheapComputeDirection(Direction* dir) const;

    





    bool cheapIsDirection(Direction dir) const {
        Direction computedDir = kUnknown_Direction;
        (void)this->cheapComputeDirection(&computedDir);
        return computedDir == dir;
    }

    enum PathAsRect {
        
        kNone_PathAsRect,
        
        kFill_PathAsRect,
        
        kStroke_PathAsRect,
    };

    







    PathAsRect asRect(Direction* direction = NULL) const;

    








    bool isRect(bool* isClosed, Direction* direction) const;

    










    bool isNestedRects(SkRect rect[2], Direction dirs[2] = NULL) const;

    





    void addRect(const SkRect& rect, Direction dir = kCW_Direction);

    













    void addRect(SkScalar left, SkScalar top, SkScalar right, SkScalar bottom,
                 Direction dir = kCW_Direction);

    






    void addOval(const SkRect& oval, Direction dir = kCW_Direction);

    











    void addCircle(SkScalar x, SkScalar y, SkScalar radius,
                   Direction dir = kCW_Direction);

    





    void addArc(const SkRect& oval, SkScalar startAngle, SkScalar sweepAngle);

    







    void addRoundRect(const SkRect& rect, SkScalar rx, SkScalar ry,
                      Direction dir = kCW_Direction);

    











    void addRoundRect(const SkRect& rect, const SkScalar radii[],
                      Direction dir = kCW_Direction);

    





    void addRRect(const SkRRect& rrect, Direction dir = kCW_Direction);

    










    void addPoly(const SkPoint pts[], int count, bool close);

    enum AddPathMode {
        

        kAppend_AddPathMode,
        





        kExtend_AddPathMode
    };

    




    void addPath(const SkPath& src, SkScalar dx, SkScalar dy,
                 AddPathMode mode = kAppend_AddPathMode);

    

    void addPath(const SkPath& src, AddPathMode mode = kAppend_AddPathMode) {
        SkMatrix m;
        m.reset();
        this->addPath(src, m, mode);
    }

    




    void addPath(const SkPath& src, const SkMatrix& matrix, AddPathMode mode = kAppend_AddPathMode);

    


    void reverseAddPath(const SkPath& src);

    





    void offset(SkScalar dx, SkScalar dy, SkPath* dst) const;

    




    void offset(SkScalar dx, SkScalar dy) {
        this->offset(dx, dy, this);
    }

    





    void transform(const SkMatrix& matrix, SkPath* dst) const;

    



    void transform(const SkMatrix& matrix) {
        this->transform(matrix, this);
    }

    





    bool getLastPt(SkPoint* lastPt) const;

    





    void setLastPt(SkScalar x, SkScalar y);

    




    void setLastPt(const SkPoint& p) {
        this->setLastPt(p.fX, p.fY);
    }

    enum SegmentMask {
        kLine_SegmentMask   = 1 << 0,
        kQuad_SegmentMask   = 1 << 1,
        kConic_SegmentMask  = 1 << 2,
        kCubic_SegmentMask  = 1 << 3,
    };

    




    uint32_t getSegmentMasks() const { return fPathRef->getSegmentMasks(); }

    enum Verb {
        kMove_Verb,     
        kLine_Verb,     
        kQuad_Verb,     
        kConic_Verb,    
        kCubic_Verb,    
        kClose_Verb,    
        kDone_Verb,     
    };

    








    class SK_API Iter {
    public:
        Iter();
        Iter(const SkPath&, bool forceClose);

        void setPath(const SkPath&, bool forceClose);

        







        Verb next(SkPoint pts[4], bool doConsumeDegerates = true) {
            if (doConsumeDegerates) {
                this->consumeDegenerateSegments();
            }
            return this->doNext(pts);
        }

        



        SkScalar conicWeight() const { return *fConicWeights; }

        







        bool isCloseLine() const { return SkToBool(fCloseLine); }

        


        bool isClosedContour() const;

    private:
        const SkPoint*  fPts;
        const uint8_t*  fVerbs;
        const uint8_t*  fVerbStop;
        const SkScalar* fConicWeights;
        SkPoint         fMoveTo;
        SkPoint         fLastPt;
        SkBool8         fForceClose;
        SkBool8         fNeedClose;
        SkBool8         fCloseLine;
        SkBool8         fSegmentState;

        inline const SkPoint& cons_moveTo();
        Verb autoClose(SkPoint pts[2]);
        void consumeDegenerateSegments();
        Verb doNext(SkPoint pts[4]);
    };

    

    class SK_API RawIter {
    public:
        RawIter();
        RawIter(const SkPath&);

        void setPath(const SkPath&);

        






        Verb next(SkPoint pts[4]);

        SkScalar conicWeight() const { return *fConicWeights; }

    private:
        const SkPoint*  fPts;
        const uint8_t*  fVerbs;
        const uint8_t*  fVerbStop;
        const SkScalar* fConicWeights;
        SkPoint         fMoveTo;
        SkPoint         fLastPt;
    };

    



    bool contains(SkScalar x, SkScalar y) const;

    void dump(SkWStream* , bool forceClose) const;
    void dump() const;

    



    size_t writeToMemory(void* buffer) const;
    







    size_t readFromMemory(const void* buffer, size_t length);

    



    uint32_t getGenerationID() const;

#ifdef SK_BUILD_FOR_ANDROID
    static const int kPathRefGenIDBitCnt = 30; 
    const SkPath* getSourcePath() const;
    void setSourcePath(const SkPath* path);
#else
    static const int kPathRefGenIDBitCnt = 32;
#endif

    SkDEBUGCODE(void validate() const;)

private:
    enum SerializationOffsets {
        
        kUnused1_SerializationShift = 28,    
        kDirection_SerializationShift = 26, 
        kUnused2_SerializationShift = 25,    
        
        kConvexity_SerializationShift = 16, 
        kFillType_SerializationShift = 8,   
        
    };

    SkAutoTUnref<SkPathRef> fPathRef;

    int                 fLastMoveToIndex;
    uint8_t             fFillType;
    mutable uint8_t     fConvexity;
    mutable uint8_t     fDirection;
#ifdef SK_BUILD_FOR_ANDROID
    const SkPath*       fSourcePath;
#endif

    



    void resetFields();

    



    void copyFields(const SkPath& that);

    friend class Iter;

    friend class SkPathStroker;

    



    void reversePathTo(const SkPath&);

    
    
    
    
    
    
    inline void injectMoveToIfNeeded();

    inline bool hasOnlyMoveTos() const;

    Convexity internalGetConvexity() const;

    bool isRectContour(bool allowPartial, int* currVerb, const SkPoint** pts,
                       bool* isClosed, Direction* direction) const;

    


    bool hasComputedBounds() const {
        SkDEBUGCODE(this->validate();)
        return fPathRef->hasComputedBounds();
    }


    
    void setBounds(const SkRect& rect) {
        SkPathRef::Editor ed(&fPathRef);

        ed.setBounds(rect);
    }

    friend class SkAutoPathBoundsUpdate;
    friend class SkAutoDisableOvalCheck;
    friend class SkAutoDisableDirectionCheck;
    friend class SkBench_AddPathTest; 
    friend class PathTest_Private; 
};

#endif
