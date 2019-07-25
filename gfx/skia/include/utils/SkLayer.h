








#ifndef SkLayer_DEFINED
#define SkLayer_DEFINED

#include "SkRefCnt.h"
#include "SkTDArray.h"
#include "SkColor.h"
#include "SkMatrix.h"
#include "SkPoint.h"
#include "SkRect.h"
#include "SkSize.h"

class SkCanvas;

class SkLayer : public SkRefCnt {

public:
    SkLayer();
    SkLayer(const SkLayer&);
    virtual ~SkLayer();

    bool isInheritFromRootTransform() const;
    SkScalar getOpacity() const { return m_opacity; }
    const SkSize& getSize() const { return m_size; }
    const SkPoint& getPosition() const { return m_position; }
    const SkPoint& getAnchorPoint() const { return m_anchorPoint; }
    const SkMatrix& getMatrix() const { return fMatrix; }
    const SkMatrix& getChildrenMatrix() const { return fChildrenMatrix; }

    SkScalar getWidth() const { return m_size.width(); }
    SkScalar getHeight() const { return m_size.height(); }

    void setInheritFromRootTransform(bool);
    void setOpacity(SkScalar opacity) { m_opacity = opacity; }
    void setSize(SkScalar w, SkScalar h) { m_size.set(w, h); }
    void setPosition(SkScalar x, SkScalar y) { m_position.set(x, y); }
    void setAnchorPoint(SkScalar x, SkScalar y) { m_anchorPoint.set(x, y); }
    void setMatrix(const SkMatrix&);
    void setChildrenMatrix(const SkMatrix&);

    

    

    int countChildren() const;

    


    SkLayer* getChild(int index) const;

    



    SkLayer* addChild(SkLayer* child);

    


    void detachFromParent();

    

    void removeChildren();

    

    SkLayer* getParent() const { return fParent; }

    


    SkLayer* getRootLayer() const;

    

    




    void getLocalTransform(SkMatrix* matrix) const;

    



    void localToGlobal(SkMatrix* matrix) const;

    

    void draw(SkCanvas*, SkScalar opacity);
    void draw(SkCanvas* canvas) {
        this->draw(canvas, SK_Scalar1);
    }

protected:
    virtual void onDraw(SkCanvas*, SkScalar opacity);

private:
    enum Flags {
        kInheritFromRootTransform_Flag = 0x01
    };

    SkLayer*    fParent;
    SkScalar    m_opacity;
    SkSize      m_size;
    SkPoint     m_position;
    SkPoint     m_anchorPoint;
    SkMatrix    fMatrix;
    SkMatrix    fChildrenMatrix;
    uint32_t    fFlags;

    SkTDArray<SkLayer*> m_children;

    typedef SkRefCnt INHERITED;
};

#endif
