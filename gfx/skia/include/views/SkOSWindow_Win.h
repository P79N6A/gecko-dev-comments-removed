








#ifndef SkOSWindow_Win_DEFINED
#define SkOSWindow_Win_DEFINED

#include "SkWindow.h"

class SkOSWindow : public SkWindow {
public:
    SkOSWindow(void* hwnd);
    virtual ~SkOSWindow();

    void*   getHWND() const { return fHWND; }
    void    setSize(int width, int height);
    void    updateSize();

    static bool PostEvent(SkEvent* evt, SkEventSinkID, SkMSec delay);
    
    bool attachGL();
    void detachGL();
    void presentGL();

    bool attachD3D9();
    void detachD3D9();
    void presentD3D9();

    void* d3d9Device() { return fD3D9Device; }

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

    void*               fHGLRC;

    bool                fGLAttached;

    void*               fD3D9Device;
    bool                fD3D9Attached;

    HMENU               fMBar;

    typedef SkWindow INHERITED; 
};

#endif

