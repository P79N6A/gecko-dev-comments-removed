








#ifndef SkView_DEFINED
#define SkView_DEFINED

#include "SkEventSink.h"
#include "SkRect.h"
#include "SkDOM.h"
#include "SkTDict.h"

class SkCanvas;
class SkLayerView;






class SkView : public SkEventSink {
public:
    enum Flag_Shift {
        kVisible_Shift,
        kEnabled_Shift,
        kFocusable_Shift,
        kFlexH_Shift,
        kFlexV_Shift,
        kNoClip_Shift,

        kFlagShiftCount
    };
    enum Flag_Mask {
        kVisible_Mask   = 1 << kVisible_Shift,      
        kEnabled_Mask   = 1 << kEnabled_Shift,      
        kFocusable_Mask = 1 << kFocusable_Shift,    
        kFlexH_Mask     = 1 << kFlexH_Shift,        
        kFlexV_Mask     = 1 << kFlexV_Shift,        
        kNoClip_Mask    = 1 << kNoClip_Shift,        

        kAllFlagMasks   = (uint32_t)(0 - 1) >> (32 - kFlagShiftCount)
    };

                SkView(uint32_t flags = 0);
    virtual     ~SkView();

    

    uint32_t    getFlags() const { return fFlags; }
    

    void        setFlags(uint32_t flags);

    

    int         isVisible() const { return fFlags & kVisible_Mask; }
    int         isEnabled() const { return fFlags & kEnabled_Mask; }
    int         isFocusable() const { return fFlags & kFocusable_Mask; }
    int         isClipToBounds() const { return !(fFlags & kNoClip_Mask); }
    
    void        setVisibleP(bool);
    void        setEnabledP(bool);
    void        setFocusableP(bool);
    void        setClipToBounds(bool);

    
    SkScalar    width() const { return fWidth; }
    
    SkScalar    height() const { return fHeight; }
    
    void        setSize(SkScalar width, SkScalar height);
    void        setSize(const SkPoint& size) { this->setSize(size.fX, size.fY); }
    void        setWidth(SkScalar width) { this->setSize(width, fHeight); }
    void        setHeight(SkScalar height) { this->setSize(fWidth, height); }
    
    void        getLocalBounds(SkRect* bounds) const;

    
    SkScalar    locX() const { return fLoc.fX; }
    
    SkScalar    locY() const { return fLoc.fY; }
    
    void        setLoc(SkScalar x, SkScalar y);
    void        setLoc(const SkPoint& loc) { this->setLoc(loc.fX, loc.fY); }
    void        setLocX(SkScalar x) { this->setLoc(x, fLoc.fY); }
    void        setLocY(SkScalar y) { this->setLoc(fLoc.fX, y); }
    
    void        offset(SkScalar dx, SkScalar dy);

    
    virtual void draw(SkCanvas* canvas);

    



    void        inval(SkRect* rectOrNull);

    

    SkView* getFocusView() const;
    bool    hasFocus() const;

    enum FocusDirection {
        kNext_FocusDirection,
        kPrev_FocusDirection,

        kFocusDirectionCount
    };
    bool    acceptFocus();
    SkView* moveFocus(FocusDirection);

    

    class Click {
    public:
        Click(SkView* target);
        virtual ~Click();

        const char* getType() const { return fType; }
        bool        isType(const char type[]) const;
        void        setType(const char type[]);     
        void        copyType(const char type[]);    

        enum State {
            kDown_State,
            kMoved_State,
            kUp_State
        };
        SkPoint     fOrig, fPrev, fCurr;
        SkIPoint    fIOrig, fIPrev, fICurr;
        State       fState;
        void*       fOwner;
    private:
        SkEventSinkID   fTargetID;
        char*           fType;
        bool            fWeOwnTheType;

        void resetType();

        friend class SkView;
    };
    Click*  findClickHandler(SkScalar x, SkScalar y);

    static void DoClickDown(Click*, int x, int y);
    static void DoClickMoved(Click*, int x, int y);
    static void DoClickUp(Click*, int x, int y);

    



    SkView*     sendEventToParents(const SkEvent&);
    



    SkView* sendQueryToParents(SkEvent*);

    

    
    SkView*     getParent() const { return fParent; }
    SkView*     attachChildToFront(SkView* child);
    



    SkView*     attachChildToBack(SkView* child);
    


    void        detachFromParent();
    



    
    void        detachAllChildren();

    

    void        globalToLocal(SkPoint* pt) const { if (pt) this->globalToLocal(pt->fX, pt->fY, pt); }
    


    void        globalToLocal(SkScalar globalX, SkScalar globalY, SkPoint* local) const;

    






    class F2BIter {
    public:
        F2BIter(const SkView* parent);
        SkView* next();
    private:
        SkView* fFirstChild, *fChild;
    };

    






    class B2FIter {
    public:
        B2FIter(const SkView* parent);
        SkView* next();
    private:
        SkView* fFirstChild, *fChild;
    };

    





    class Artist : public SkRefCnt {
    public:
        void draw(SkView*, SkCanvas*);
        void inflate(const SkDOM&, const SkDOM::Node*);
    protected:
        virtual void onDraw(SkView*, SkCanvas*) = 0;
        virtual void onInflate(const SkDOM&, const SkDOM::Node*);
    };
    


    Artist* getArtist() const;
    



    Artist* setArtist(Artist* artist);

    





    class Layout : public SkRefCnt {
    public:
        void layoutChildren(SkView* parent);
        void inflate(const SkDOM&, const SkDOM::Node*);
    protected:
        virtual void onLayoutChildren(SkView* parent) = 0;
        virtual void onInflate(const SkDOM&, const SkDOM::Node*);
    };

    


    Layout* getLayout() const;
    



    Layout* setLayout(Layout*, bool invokeLayoutNow = true);
    

    void    invokeLayout();

    

    void    inflate(const SkDOM& dom, const SkDOM::Node* node);
    






    void    postInflate(const SkTDict<SkView*>& ids);

    SkDEBUGCODE(void dump(bool recurse) const;)

protected:
    
    virtual void    onDraw(SkCanvas*);
    
    virtual void    onSizeChange();
    



    virtual bool    handleInval(const SkRect*);
    
    virtual SkCanvas* beforeChildren(SkCanvas* c) { return c; }
    
    virtual void afterChildren(SkCanvas* orig) {}

    
    virtual void beforeChild(SkView* child, SkCanvas* canvas) {}
    
    virtual void afterChild(SkView* child, SkCanvas* canvas) {}

    

    virtual Click* onFindClickHandler(SkScalar x, SkScalar y);
    




    virtual bool onSendClickToChildren(SkScalar x, SkScalar y);
    


    virtual bool    onClick(Click*);
    
    virtual void    onInflate(const SkDOM& dom, const SkDOM::Node* node);
    


    virtual void    onPostInflate(const SkTDict<SkView*>&);

public:
    
    virtual void    onFocusChange(bool gainFocusP);
protected:

    
    virtual bool    onGetFocusView(SkView**) const { return false; }
    virtual bool    onSetFocusView(SkView*) { return false; }

private:
    SkScalar    fWidth, fHeight;
    SkPoint     fLoc;
    SkView*     fParent;
    SkView*     fFirstChild;
    SkView*     fNextSibling;
    SkView*     fPrevSibling;
    uint8_t     fFlags;
    uint8_t     fContainsFocus;

    friend class B2FIter;
    friend class F2BIter;
    
    friend class SkLayerView;

    bool    setFocusView(SkView* fvOrNull);
    SkView* acceptFocus(FocusDirection);
    void    detachFromParent_NoLayout();
};

#endif

