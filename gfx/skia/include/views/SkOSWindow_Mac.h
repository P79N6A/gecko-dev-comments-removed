







#ifndef SkOSWindow_MacCocoa_DEFINED
#define SkOSWindow_MacCocoa_DEFINED

#include "SkWindow.h"

class SkOSWindow : public SkWindow {
public:
    SkOSWindow(void* hwnd);
    ~SkOSWindow();
    void*   getHWND() const { return fHWND; }
    
    virtual bool onDispatchClick(int x, int y, Click::State state, 
                                 void* owner);
    void    detachGL();
    bool    attachGL();
    void    presentGL();
    
protected:
    
    virtual bool onEvent(const SkEvent& evt);
    
    virtual void onHandleInval(const SkIRect&);
    
    virtual void onAddMenu(const SkOSMenu*);
    virtual void onUpdateMenu(const SkOSMenu*);
    virtual void onSetTitle(const char[]);
    
private:
    void*   fHWND;
    bool    fInvalEventIsPending;
    void*   fNotifier;
    void*   fGLContext;
    typedef SkWindow INHERITED;
};

#endif