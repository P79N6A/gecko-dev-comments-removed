








#ifndef SkWidgetViews_DEFINED
#define SkWidgetViews_DEFINED

#include "SkView.h"


enum SkWidgetEnum {
    kBorder_WidgetEnum,         
    kButton_WidgetEnum,         
    kImage_WidgetEnum,          
    kList_WidgetEnum,           
    kProgress_WidgetEnum,       
    kScroll_WidgetEnum,         
    kText_WidgetEnum,           
    
    kWidgetEnumCount
};


enum SkinEnum {
    kBorder_SkinEnum,
    kButton_SkinEnum,
    kProgress_SkinEnum,
    kScroll_SkinEnum,
    kStaticText_SkinEnum,
    
    kSkinEnumCount
};

#include "SkAnimator.h"

const char* get_skin_enum_path(SkinEnum se);
void init_skin_anim(const char path[], SkAnimator* anim);
void init_skin_anim(SkinEnum se, SkAnimator* anim);
void init_skin_paint(SkinEnum se, SkPaint* paint);
void inflate_paint(const SkDOM& dom, const SkDOM::Node* node, SkPaint* paint);




SkView* SkWidgetFactory(SkWidgetEnum);




SkView* SkWidgetFactory(const char name[]);



class SkWidgetView : public SkView {
public:
    SkWidgetView();

    const char* getLabel() const;
    void        getLabel(SkString* label) const;

    void        setLabel(const char[]);
    void        setLabel(const char[], size_t len);
    void        setLabel(const SkString&);

    SkEvent&        event() { return fEvent; }
    const SkEvent&  event() const { return fEvent; }

    

    bool    postWidgetEvent();
    
    

    static SkEventSinkID GetWidgetEventSinkID(const SkEvent&);

protected:
    


    virtual void onLabelChange(const char oldLabel[], const char newLabel[]);
    





    virtual bool onPrepareWidgetEvent(SkEvent* evt);

    
    virtual void onInflate(const SkDOM& dom, const SkDOM::Node*);
    
private:
    SkString    fLabel;
    SkEvent     fEvent;
    
    typedef SkView INHERITED;
};



class SkButtonView : public SkWidgetView {
public:
    
    
protected:
    
    virtual bool onEvent(const SkEvent&);
private:
    typedef SkWidgetView INHERITED;
};



class SkCheckButtonView : public SkWidgetView {
public:
    SkCheckButtonView();

    
    
    enum CheckState {
        kOff_CheckState,        
        kOn_CheckState,         
        kUnknown_CheckState     
    };
    CheckState  getCheckState() const { return (CheckState)fCheckState; }
    void        setCheckState(CheckState);

    




    static bool GetWidgetEventCheckState(const SkEvent&, CheckState* state);

protected:
    
    virtual void onCheckStateChange(CheckState oldState, CheckState newState);

    
    virtual void onInflate(const SkDOM& dom, const SkDOM::Node*);
    virtual bool onPrepareWidgetEvent(SkEvent* evt);
    
private:
    uint8_t  fCheckState;
    
    typedef SkWidgetView INHERITED;
};


#include "SkTextBox.h"

class SkStaticTextView : public SkView {
public:
            SkStaticTextView();
    virtual ~SkStaticTextView();

    enum Mode {
        kFixedSize_Mode,
        kAutoWidth_Mode,
        kAutoHeight_Mode,

        kModeCount
    };
    Mode    getMode() const { return (Mode)fMode; }
    void    setMode(Mode);

    SkTextBox::SpacingAlign getSpacingAlign() const { return (SkTextBox::SpacingAlign)fSpacingAlign; }
    void    setSpacingAlign(SkTextBox::SpacingAlign);

    void    getMargin(SkPoint* margin) const;
    void    setMargin(SkScalar dx, SkScalar dy);

    size_t  getText(SkString* text = NULL) const;
    size_t  getText(char text[] = NULL) const;
    void    setText(const SkString&);
    void    setText(const char text[]);
    void    setText(const char text[], size_t len);

    void    getPaint(SkPaint*) const;
    void    setPaint(const SkPaint&);

protected:
    
    virtual void onDraw(SkCanvas*);
    virtual void onInflate(const SkDOM& dom, const SkDOM::Node*);

private:
    SkPoint     fMargin;
    SkString    fText;
    SkPaint     fPaint;
    uint8_t     fMode;
    uint8_t     fSpacingAlign;

    void computeSize();

    typedef SkView INHERITED;
};



class SkAnimator;
class SkListSource;
class SkScrollBarView;

class SkListView : public SkWidgetView {
public:
            SkListView();
    virtual ~SkListView();

    bool    hasScrollBar() const { return fScrollBar != NULL; }
    void    setHasScrollBar(bool);
    
    

    int     getVisibleRowCount() const { return fVisibleRowCount; }
    

    int     getSelection() const { return fCurrIndex; }
    

    void    setSelection(int);
    



    bool    moveSelectionUp();
    



    bool    moveSelectionDown();

    SkListSource*   getListSource() const { return fSource; }
    SkListSource*   setListSource(SkListSource*);

    



    static int GetWidgetEventListIndex(const SkEvent&);

protected:
    
    virtual void onDraw(SkCanvas*);
    virtual void onSizeChange();
    virtual bool onEvent(const SkEvent&);
    virtual void onInflate(const SkDOM& dom, const SkDOM::Node* node);
    virtual bool onPrepareWidgetEvent(SkEvent*);

private:
    enum DirtyFlags {
        kAnimCount_DirtyFlag    = 0x01,
        kAnimContent_DirtyFlag  = 0x02
    };
    void    dirtyCache(unsigned dirtyFlags);
    bool    ensureCache();

    int     logicalToVisualIndex(int index) const { return index - fScrollIndex; }
    void    invalSelection();
    SkScalar getContentWidth() const;
    bool    getRowRect(int index, SkRect*) const;
    void    ensureSelectionIsVisible();
    void    ensureVisibleRowCount();

    struct BindingRec;

    enum Heights {
        kNormal_Height,
        kSelected_Height
    };
    SkListSource*   fSource;
    SkScrollBarView*    fScrollBar;
    SkAnimator*     fAnims;
    BindingRec*     fBindings;
    SkString        fSkinName;
    SkScalar        fHeights[2];
    int16_t         fScrollIndex, fCurrIndex;
    uint16_t        fVisibleRowCount, fBindingCount;
    SkBool8         fAnimContentDirty;
    SkBool8         fAnimFocusDirty;

    typedef SkWidgetView INHERITED;
};

class SkListSource : public SkRefCnt {
public:
    virtual int countFields();
    virtual void getFieldName(int index, SkString* field);
    
    virtual int findFieldIndex(const char field[]);

    virtual int countRecords();
    virtual void getRecord(int rowIndex, int fieldIndex, SkString* data);

    virtual bool prepareWidgetEvent(SkEvent*, int rowIndex);
    
    static SkListSource* Factory(const char name[]);
};

#endif
