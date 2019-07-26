






#ifndef SkRRect_DEFINED
#define SkRRect_DEFINED

#include "SkRect.h"
#include "SkPoint.h"

class SkPath;






































class SK_API SkRRect {
public:
    



    enum Type {
        
        kUnknown_Type = -1,

        
        kEmpty_Type,

        
        
        kRect_Type,

        
        
        kOval_Type,

        
        
        
        kSimple_Type,

        
        
        
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
    inline bool isComplex() const { return kComplex_Type == this->getType(); }

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

    








    bool contains(const SkPoint& p) const {
        return contains(p.fX, p.fY);
    }

    








    bool contains(SkScalar x, SkScalar y) const;

    







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

    SkDEBUGCODE(void validate() const;)

    enum {
        kSizeInMemory = 12 * sizeof(SkScalar)
    };

    




    uint32_t writeToMemory(void* buffer) const;

    




    uint32_t readFromMemory(const void* buffer);

private:
    SkRect fRect;
    
    SkVector fRadii[4];
    mutable Type fType;
    
    

    void computeType() const;

    
    friend class SkPath;
};

#endif
