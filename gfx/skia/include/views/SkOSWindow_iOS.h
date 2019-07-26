






#ifndef SkOSWindow_iOS_DEFINED
#define SkOSWindow_iOS_DEFINED

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
        kNativeGL_BackEndType,
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
    typedef SkWindow INHERITED;
};

#endif

