







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
    enum SkBackEndTypes {
        kNone_BackEndType,
#if SK_SUPPORT_GPU
        kNativeGL_BackEndType,
#endif
    };

    void    detach();
    bool    attach(SkBackEndTypes attachType, int msaaSampleCount);
    void    present();

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
#if SK_SUPPORT_GPU
    void*   fGLContext;
#endif
    typedef SkWindow INHERITED;
};

#endif
