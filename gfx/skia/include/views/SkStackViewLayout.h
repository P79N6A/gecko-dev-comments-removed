








#ifndef SkStackViewLayout_DEFINED
#define SkStackViewLayout_DEFINED

#include "SkView.h"

class SkStackViewLayout : public SkView::Layout {
public:
    SkStackViewLayout();

    enum Orient {
        kHorizontal_Orient,
        kVertical_Orient,

        kOrientCount
    };
    Orient  getOrient() const { return (Orient)fOrient; }
    void    setOrient(Orient);

    void        getMargin(SkRect*) const;
    void        setMargin(const SkRect&);

    SkScalar    getSpacer() const { return fSpacer; }
    void        setSpacer(SkScalar);

    

    enum Pack {
        kStart_Pack,
        kCenter_Pack,
        kEnd_Pack,
        
        kPackCount
    };
    Pack    getPack() const { return (Pack)fPack; }
    void    setPack(Pack);

    

    enum Align {
        kStart_Align,
        kCenter_Align,
        kEnd_Align,
        kStretch_Align,

        kAlignCount
    };
    Align   getAlign() const { return (Align)fAlign; }
    void    setAlign(Align);

    bool    getRound() const { return SkToBool(fRound); }
    void    setRound(bool);

protected:
    virtual void onLayoutChildren(SkView* parent);
    virtual void onInflate(const SkDOM&, const SkDOM::Node*);

private:
    SkRect      fMargin;
    SkScalar    fSpacer;
    uint8_t     fOrient, fPack, fAlign, fRound;
};

class SkFillViewLayout : public SkView::Layout {
public:
            SkFillViewLayout();
    void    getMargin(SkRect*) const;
    void    setMargin(const SkRect&);

protected:
    
    virtual void onLayoutChildren(SkView* parent);
    virtual void onInflate(const SkDOM& dom, const SkDOM::Node* node);

private:
    SkRect  fMargin;
    typedef SkView::Layout INHERITED;
};

#endif

