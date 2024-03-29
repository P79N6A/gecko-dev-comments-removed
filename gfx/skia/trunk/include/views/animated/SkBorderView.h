








#ifndef SkBorderView_DEFINED
#define SkBorderView_DEFINED

#include "SkView.h"
#include "SkWidgetViews.h"
#include "SkAnimator.h"

class SkBorderView : public SkWidgetView {
public:
    SkBorderView();
    ~SkBorderView();
    void setSkin(const char skin[]);
    SkScalar getLeft() const { return fLeft; }
    SkScalar getRight() const { return fRight; }
    SkScalar getTop() const { return fTop; }
    SkScalar getBottom() const { return fBottom; }
protected:
    
    virtual void onInflate(const SkDOM& dom,  const SkDOM::Node* node);
    virtual void onSizeChange();
    virtual void onDraw(SkCanvas* canvas);
    virtual bool onEvent(const SkEvent& evt);
private:
    SkAnimator fAnim;
    SkScalar fLeft, fRight, fTop, fBottom;  
    SkRect fMargin;

    typedef SkWidgetView INHERITED;
};

#endif
