






#ifndef SkTextLayout_DEFINED
#define SkTextLayout_DEFINED

#include "SkPaint.h"
#include "SkRefCnt.h"

class SkTextStyle : public SkRefCnt {
public:
    SkTextStyle();
    SkTextStyle(const SkTextStyle&);
    explicit SkTextStyle(const SkPaint&);
    virtual ~SkTextStyle();

    const SkPaint& paint() const { return fPaint; }
    SkPaint& paint() { return fPaint; }
    
    

private:
    SkPaint fPaint;
};

class SkTextLayout {
public:
    SkTextLayout();
    ~SkTextLayout();

    void setText(const char text[], size_t length);
    void setBounds(const SkRect& bounds);

    SkTextStyle* getDefaultStyle() const { return fDefaultStyle; }
    SkTextStyle* setDefaultStyle(SkTextStyle*);



    void draw(SkCanvas* canvas);

private:
    SkTDArray<char> fText;
    SkTextStyle*    fDefaultStyle;
    SkRect          fBounds;

    
    struct Line;
    struct GlyphRun;
    SkTDArray<Line*> fLines;
};

#endif

