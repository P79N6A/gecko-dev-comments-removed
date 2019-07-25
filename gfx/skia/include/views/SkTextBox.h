








#ifndef SkTextBox_DEFINED
#define SkTextBox_DEFINED

#include "SkCanvas.h"












class SkTextBox {
public:
    SkTextBox();

    enum Mode {
        kOneLine_Mode,
        kLineBreak_Mode,

        kModeCount
    };
    Mode    getMode() const { return (Mode)fMode; }
    void    setMode(Mode);

    enum SpacingAlign {
        kStart_SpacingAlign,
        kCenter_SpacingAlign,
        kEnd_SpacingAlign,

        kSpacingAlignCount
    };
    SpacingAlign    getSpacingAlign() const { return (SpacingAlign)fSpacingAlign; }
    void            setSpacingAlign(SpacingAlign);

    void    getBox(SkRect*) const;
    void    setBox(const SkRect&);
    void    setBox(SkScalar left, SkScalar top, SkScalar right, SkScalar bottom);

    void    getSpacing(SkScalar* mul, SkScalar* add) const;
    void    setSpacing(SkScalar mul, SkScalar add);

    void    draw(SkCanvas*, const char text[], size_t len, const SkPaint&);

    void    setText(const char text[], size_t len, const SkPaint&);
    void    draw(SkCanvas*);
    int     countLines() const;
    SkScalar getTextHeight() const;

private:
    SkRect      fBox;
    SkScalar    fSpacingMul, fSpacingAdd;
    uint8_t     fMode, fSpacingAlign;
    const char* fText;
    size_t      fLen;
    const SkPaint* fPaint;
};

class SkTextLineBreaker {
public:
    static int CountLines(const char text[], size_t len, const SkPaint&, SkScalar width);
};

#endif

