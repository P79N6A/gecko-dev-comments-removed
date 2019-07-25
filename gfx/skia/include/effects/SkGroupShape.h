






#ifndef SkGroupShape_DEFINED
#define SkGroupShape_DEFINED

#include "SkMatrix.h"
#include "SkShape.h"
#include "SkTDArray.h"
#include "SkThread.h"

template <typename T> class SkTRefCnt : public T {
public:
    SkTRefCnt() : fRefCnt(1) {}
    ~SkTRefCnt() { SkASSERT(1 == fRefCnt); }

    int32_t getRefCnt() const { return fRefCnt; }
    
    

    void ref() const {
        SkASSERT(fRefCnt > 0);
        sk_atomic_inc(&fRefCnt);
    }
    
    




    void unref() const {
        SkASSERT(fRefCnt > 0);
        if (sk_atomic_dec(&fRefCnt) == 1) {
            fRefCnt = 1;    
            SkDELETE(this);
        }
    }
    
    static void SafeRef(const SkTRefCnt* obj) {
        if (obj) {
            obj->ref();
        }
    }
    
    static void SafeUnref(const SkTRefCnt* obj) {
        if (obj) {
            obj->unref();
        }
    }
    
private:
    mutable int32_t fRefCnt;
};

class SkMatrixRef : public SkTRefCnt<SkMatrix> {
public:
    SkMatrixRef() { this->reset(); }
    explicit SkMatrixRef(const SkMatrix& matrix) {
        SkMatrix& m = *this;
        m = matrix;
    }
    
    SkMatrix& operator=(const SkMatrix& matrix) {
        SkMatrix& m = *this;
        m = matrix;
        return m;
    }
};

class SkGroupShape : public SkShape {
public:
            SkGroupShape();
    virtual ~SkGroupShape();

    

    int countShapes() const;

    


    SkShape* getShape(int index, SkMatrixRef** = NULL) const;
    
    

    SkMatrixRef* getShapeMatrixRef(int index) const {
        SkMatrixRef* mr = NULL;
        (void)this->getShape(index, &mr);
        return mr;
    }

    








    void addShape(int index, SkShape*, SkMatrixRef* = NULL);

    void addShape(int index, SkShape* shape, const SkMatrix& matrix) {
        SkMatrixRef* mr = SkNEW_ARGS(SkMatrixRef, (matrix));
        this->addShape(index, shape, mr);
        mr->unref();
    }

    

    SkShape* appendShape(SkShape* shape, SkMatrixRef* mr = NULL) {
        this->addShape(this->countShapes(), shape, mr);
        return shape;
    }
    
    SkShape* appendShape(SkShape* shape, const SkMatrix& matrix) {
        this->addShape(this->countShapes(), shape, matrix);
        return shape;
    }
    
    


    void removeShape(int index);

    

    void removeAllShapes();

    
    virtual Factory getFactory();
    virtual void flatten(SkFlattenableWriteBuffer&);

    
    static SkFlattenable* CreateProc(SkFlattenableReadBuffer&);

protected:
    
    virtual void onDraw(SkCanvas*);

    SkGroupShape(SkFlattenableReadBuffer&);

private:
    struct Rec {
        SkShape*     fShape;
        SkMatrixRef* fMatrixRef;
    };
    SkTDArray<Rec> fList;

    typedef SkShape INHERITED;
};

#endif
