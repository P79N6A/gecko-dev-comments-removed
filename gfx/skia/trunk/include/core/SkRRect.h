






#ifndef SkRRect_DEFINED
#define SkRRect_DEFINED

#include "SkRect.h"
#include "SkPoint.h"

class SkPath;
class SkMatrix;
































class SK_API SkRRect {
public:
    



    enum Type {
        
        kUnknown_Type = -1,

        
        kEmpty_Type,

        
        
        kRect_Type,

        
        
        kOval_Type,

        
        
        
        kSimple_Type,

        
        
        
        
        
        
        kNinePatch_Type,

        
        
        
        kComplex_Type,
    };

    


    Type getType() const {
        SkDEBUGCODE(this->validate();)

        if (kUnknown_Type == fType) {
            this->computeType();
        }
        SkASSERT(kUnknown_Type != fType);
        return fType;
    }

    Type type() const { return this->getType(); }

    inline bool isEmpty() const { return kEmpty_Type == this->getType(); }
    inline bool isRect() const { return kRect_Type == this->getType(); }
    inline bool isOval() const { return kOval_Type == this->getType(); }
    inline bool isSimple() const { return kSimple_Type == this->getType(); }
    inline bool isSimpleCircular() const {
        return this->isSimple() && fRadii[0].fX == fRadii[0].fY;
    }
    inline bool isNinePatch() const { return kNinePatch_Type == this->getType(); }
    inline bool isComplex() const { return kComplex_Type == this->getType(); }

    bool allCornersCircular() const;

    SkScalar width() const { return fRect.width(); }
    SkScalar height() const { return fRect.height(); }

    


    void setEmpty() {
        fRect.setEmpty();
        memset(fRadii, 0, sizeof(fRadii));
        fType = kEmpty_Type;

        SkDEBUGCODE(this->validate();)
    }

    


    void setRect(const SkRect& rect) {
        if (rect.isEmpty()) {
            this->setEmpty();
            return;
        }

        fRect = rect;
        memset(fRadii, 0, sizeof(fRadii));
        fType = kRect_Type;

        SkDEBUGCODE(this->validate();)
    }

    



    void setOval(const SkRect& oval) {
        if (oval.isEmpty()) {
            this->setEmpty();
            return;
        }

        SkScalar xRad = SkScalarHalf(oval.width());
        SkScalar yRad = SkScalarHalf(oval.height());

        fRect = oval;
        for (int i = 0; i < 4; ++i) {
            fRadii[i].set(xRad, yRad);
        }
        fType = kOval_Type;

        SkDEBUGCODE(this->validate();)
    }

    


    void setRectXY(const SkRect& rect, SkScalar xRad, SkScalar yRad);

    


    void setNinePatch(const SkRect& rect, SkScalar leftRad, SkScalar topRad,
                      SkScalar rightRad, SkScalar bottomRad);

    


    void setRectRadii(const SkRect& rect, const SkVector radii[4]);

    
    enum Corner {
        kUpperLeft_Corner,
        kUpperRight_Corner,
        kLowerRight_Corner,
        kLowerLeft_Corner
    };

    const SkRect& rect() const { return fRect; }
    const SkVector& radii(Corner corner) const { return fRadii[corner]; }
    const SkRect& getBounds() const { return fRect; }

    



    const SkVector& getSimpleRadii() const {
        SkASSERT(!this->isComplex());
        return fRadii[0];
    }

    friend bool operator==(const SkRRect& a, const SkRRect& b) {
        return a.fRect == b.fRect &&
               SkScalarsEqual(a.fRadii[0].asScalars(),
                              b.fRadii[0].asScalars(), 8);
    }

    friend bool operator!=(const SkRRect& a, const SkRRect& b) {
        return a.fRect != b.fRect ||
               !SkScalarsEqual(a.fRadii[0].asScalars(),
                               b.fRadii[0].asScalars(), 8);
    }

    







    void inset(SkScalar dx, SkScalar dy, SkRRect* dst) const;

    void inset(SkScalar dx, SkScalar dy) {
        this->inset(dx, dy, this);
    }

    







    void outset(SkScalar dx, SkScalar dy, SkRRect* dst) const {
        this->inset(-dx, -dy, dst);
    }
    void outset(SkScalar dx, SkScalar dy) {
        this->inset(-dx, -dy, this);
    }

    


    void offset(SkScalar dx, SkScalar dy) {
        fRect.offset(dx, dy);
    }

    



    bool contains(const SkRect& rect) const;

    SkDEBUGCODE(void validate() const;)

    enum {
        kSizeInMemory = 12 * sizeof(SkScalar)
    };

    




    size_t writeToMemory(void* buffer) const;

    










    size_t readFromMemory(const void* buffer, size_t length);

    








    bool transform(const SkMatrix& matrix, SkRRect* dst) const;

#ifdef SK_DEVELOPER
    



    void dump() const;
#endif

private:
    SkRect fRect;
    
    SkVector fRadii[4];
    mutable Type fType;
    
    

    void computeType() const;
    bool checkCornerContainment(SkScalar x, SkScalar y) const;

    
    friend class SkPath;
};

#endif
