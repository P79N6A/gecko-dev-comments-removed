








#ifndef SkPath_DEFINED
#define SkPath_DEFINED

#include "SkMatrix.h"
#include "SkTDArray.h"

#ifdef ANDROID
#define GEN_ID_INC              fGenerationID++
#define GEN_ID_PTR_INC(ptr)     ptr->fGenerationID++
#else
#define GEN_ID_INC
#define GEN_ID_PTR_INC(ptr)
#endif

class SkReader32;
class SkWriter32;
class SkAutoPathBoundsUpdate;
class SkString;






class SK_API SkPath {
public:
    SkPath();
    SkPath(const SkPath&);
    ~SkPath();

    SkPath& operator=(const SkPath&);
    
    friend bool operator==(const SkPath&, const SkPath&);
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
        GEN_ID_INC;
    }

    
    bool isInverseFillType() const { return (fFillType & 2) != 0; }

    



    void toggleInverseFillType() {
        fFillType ^= 2;
        GEN_ID_INC;
     }

    enum Convexity {
        kUnknown_Convexity,
        kConvex_Convexity,
        kConcave_Convexity
    };

    




    Convexity getConvexity() const {
        if (kUnknown_Convexity == fConvexity) {
            fConvexity = (uint8_t)ComputeConvexity(*this);
        }
        return (Convexity)fConvexity;
    }

    





    Convexity getConvexityOrUnknown() const { return (Convexity)fConvexity; }

    








    void setConvexity(Convexity);

    











    static Convexity ComputeConvexity(const SkPath&);

    




    bool isConvex() const {
        return kConvex_Convexity == this->getConvexity();
    }

    






    void setIsConvex(bool isConvex) {
        this->setConvexity(isConvex ? kConvex_Convexity : kConcave_Convexity);
    }

    



    void reset();
    
    




    void rewind();

    



    bool isEmpty() const;

    







    bool isRect(SkRect* rect) const;

    

    int countPoints() const {
        return this->getPoints(NULL, 0);
    }

    



    SkPoint getPoint(int index) const;

    





    int getPoints(SkPoint points[], int max) const;

    
    void swap(SkPath& other);

    




    const SkRect& getBounds() const {
        if (fBoundsIsDirty) {
            this->computeBounds();
        }
        return fBounds;
    }

    




    void updateBoundsCache() const {
        
        this->getBounds();
    }

    

    





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

    










    void cubicTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2,
                 SkScalar x3, SkScalar y3);

    







    void cubicTo(const SkPoint& p1, const SkPoint& p2, const SkPoint& p3) {
        this->cubicTo(p1.fX, p1.fY, p2.fX, p2.fY, p3.fX, p3.fY);
    }

    
















    void    rCubicTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2,
                     SkScalar x3, SkScalar y3);

    











    void    arcTo(const SkRect& oval, SkScalar startAngle, SkScalar sweepAngle,
                  bool forceMoveTo);

    


    void arcTo(SkScalar x1, SkScalar y1, SkScalar x2, SkScalar y2,
               SkScalar radius);

    


    void arcTo(const SkPoint p1, const SkPoint p2, SkScalar radius) {
        this->arcTo(p1.fX, p1.fY, p2.fX, p2.fY, radius);
    }

    


    void close();

    enum Direction {
        
        kCW_Direction,
        
        kCCW_Direction
    };

    



    void    addRect(const SkRect& rect, Direction dir = kCW_Direction);

    











    void addRect(SkScalar left, SkScalar top, SkScalar right, SkScalar bottom,
                 Direction dir = kCW_Direction);

    




    void addOval(const SkRect& oval, Direction dir = kCW_Direction);

    









    void addCircle(SkScalar x, SkScalar y, SkScalar radius,
                   Direction dir = kCW_Direction);

    





    void addArc(const SkRect& oval, SkScalar startAngle, SkScalar sweepAngle);

    





    void    addRoundRect(const SkRect& rect, SkScalar rx, SkScalar ry,
                         Direction dir = kCW_Direction);

    






    void addRoundRect(const SkRect& rect, const SkScalar radii[],
                      Direction dir = kCW_Direction);

    




    void    addPath(const SkPath& src, SkScalar dx, SkScalar dy);

    

    void addPath(const SkPath& src) {
        SkMatrix m;
        m.reset();
        this->addPath(src, m);
    }

    


    void addPath(const SkPath& src, const SkMatrix& matrix);

    





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
        kCubic_SegmentMask  = 1 << 2
    };

    




    uint32_t getSegmentMasks() const { return fSegmentMask; }

    enum Verb {
        kMove_Verb,     
        kLine_Verb,     
        kQuad_Verb,     
        kCubic_Verb,    
        kClose_Verb,    
        kDone_Verb      
    };

    


    class SK_API Iter {
    public:
                Iter();
                Iter(const SkPath&, bool forceClose);

        void setPath(const SkPath&, bool forceClose);

        





        Verb next(SkPoint pts[4]);

        







        bool isCloseLine() const { return SkToBool(fCloseLine); }
        
        


        bool isClosedContour() const;

    private:
        const SkPoint*  fPts;
        const uint8_t*  fVerbs;
        const uint8_t*  fVerbStop;
        SkPoint         fMoveTo;
        SkPoint         fLastPt;
        SkBool8         fForceClose;
        SkBool8         fNeedClose;
        SkBool8         fNeedMoveTo;
        SkBool8         fCloseLine;

        bool cons_moveTo(SkPoint pts[1]);
        Verb autoClose(SkPoint pts[2]);
    };

    void dump(bool forceClose, const char title[] = NULL) const;
    void dump() const;

    void flatten(SkWriter32&) const;
    void unflatten(SkReader32&);

#ifdef ANDROID
    uint32_t getGenerationID() const;
#endif

    SkDEBUGCODE(void validate() const;)

private:
    SkTDArray<SkPoint>  fPts;
    SkTDArray<uint8_t>  fVerbs;
    mutable SkRect      fBounds;
    uint8_t             fFillType;
    uint8_t             fSegmentMask;
    mutable uint8_t     fBoundsIsDirty;
    mutable uint8_t     fConvexity;
#ifdef ANDROID
    uint32_t            fGenerationID;
#endif

    
    void computeBounds() const;

    friend class Iter;
    void cons_moveto();

    friend class SkPathStroker;
    



    void pathTo(const SkPath& path);

    



    void reversePathTo(const SkPath&);

    friend const SkPoint* sk_get_path_points(const SkPath&, int index);
    friend class SkAutoPathBoundsUpdate;
};

#endif

