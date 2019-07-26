








#ifndef SkOSWindow_Win_DEFINED
#define SkOSWindow_Win_DEFINED

#include "SkWindow.h"

#if SK_ANGLE
#include "EGL/egl.h"
#endif

class SkOSWindow : public SkWindow {
public:
    SkOSWindow(void* hwnd);
    virtual ~SkOSWindow();

    void*   getHWND() const { return fHWND; }
    void    setSize(int width, int height);
    void    updateSize();

    static bool PostEvent(SkEvent* evt, SkEventSinkID, SkMSec delay);

    enum SkBackEndTypes {
        kNone_BackEndType,
#if SK_SUPPORT_GPU
        kNativeGL_BackEndType,
#if SK_ANGLE
        kANGLE_BackEndType,
#endif 
#endif 
    };

    bool attach(SkBackEndTypes attachType, int msaaSampleCount);
    void detach();
    void present();

    bool wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static bool QuitOnDeactivate(HWND hWnd);

    enum {
        SK_WM_SkEvent = WM_APP + 1000,
        SK_WM_SkTimerID = 0xFFFF    
    };

protected:
    virtual bool quitOnDeactivate() { return true; }

    
    virtual void onHandleInval(const SkIRect&);
    
    virtual void onAddMenu(const SkOSMenu*);

    virtual void onSetTitle(const char title[]);

private:
    void*               fHWND;

    void                doPaint(void* ctx);

#if SK_SUPPORT_GPU
    void*               fHGLRC;
#if SK_ANGLE
    EGLDisplay          fDisplay;
    EGLContext          fContext;
    EGLSurface          fSurface;
#endif 
#endif 

    HMENU               fMBar;

    SkBackEndTypes      fAttached;

#if SK_SUPPORT_GPU
    bool attachGL(int msaaSampleCount);
    void detachGL();
    void presentGL();

#if SK_ANGLE
    bool attachANGLE(int msaaSampleCount);
    void detachANGLE();
    void presentANGLE();
#endif 
#endif 

    typedef SkWindow INHERITED;
};

#endif
